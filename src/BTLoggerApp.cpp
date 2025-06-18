#include "BTLoggerApp.hpp"
#include "UI/UIScale.hpp"
#include "UI/TouchManager.hpp"
#include "UI/ToastManager.hpp"
#include "UI/ScreenManager.hpp"
#include "UI/Screens/MainMenuScreen.hpp"
#include "UI/Screens/LogViewerScreen.hpp"
#include "UI/Screens/SystemInfoScreen.hpp"
#include "UI/Screens/DeviceManagerScreen.hpp"
#include "UI/Screens/FileBrowserScreen.hpp"
#include "UI/Screens/SettingsScreen.hpp"
#include "UI/CriticalErrorHandler.hpp"
#include "Core/CoreTaskManager.hpp"
#include "Core/BluetoothManager.hpp"
#include "Core/SDCardManager.hpp"

namespace BTLogger {

BTLoggerApp::BTLoggerApp()
    : coreTaskManager(nullptr), running(false), initialized(false), lastUpdate(0) {
}

BTLoggerApp::~BTLoggerApp() {
    stop();

    // Cleanup UI components
    UI::ScreenManager::cleanup();

    delete coreTaskManager;
}

bool BTLoggerApp::initialize() {
    if (initialized) {
        return true;
    }

    Serial.begin(115200);
    Serial.println("=================================");
    Serial.println("BTLogger - Bluetooth Log Receiver");
    Serial.println("=================================");

    // Initialize hardware
    setupHardware();
    lcd.init();

    // Initialize core task manager
    Serial.println("Initializing CoreTaskManager...");
    coreTaskManager = new Core::CoreTaskManager();
    if (!coreTaskManager->initialize()) {
        Serial.println("ERROR: CoreTaskManager initialization failed");
        return false;
    }

    // Initialize UI systems (on Core 1)
    UI::UIScale::initialize();
    if (!UI::TouchManager::initialize(lcd)) {
        Serial.println("ERROR: Touch initialization failed");
        return false;
    }
    UI::ToastManager::initialize(lcd);
    UI::CriticalErrorHandler::initialize(lcd);

    // Skip initial calibration to avoid infinite loop
    // Touch calibration can be done later from the Settings screen
    if (UI::TouchManager::needsCalibration()) {
        Serial.println("Touch calibration needed - can be done from Settings screen");
    }

    // Initialize screen system
    UI::ScreenManager::initialize(lcd);

    // Register screens
    UI::ScreenManager::registerScreen(new UI::Screens::MainMenuScreen());
    UI::ScreenManager::registerScreen(new UI::Screens::LogViewerScreen());
    UI::ScreenManager::registerScreen(new UI::Screens::SystemInfoScreen());

    // Create and connect DeviceManager to BluetoothManager
    auto deviceManager = new UI::Screens::DeviceManagerScreen();
    deviceManager->setBluetoothManager(coreTaskManager->getBluetoothManager());
    UI::ScreenManager::registerScreen(deviceManager);

    // Create and connect FileBrowser to SDCardManager
    auto fileBrowser = new UI::Screens::FileBrowserScreen();
    fileBrowser->setSDCardManager(coreTaskManager->getSDCardManager());
    UI::ScreenManager::registerScreen(fileBrowser);

    UI::ScreenManager::registerScreen(new UI::Screens::SettingsScreen());

    // Start with main menu
    UI::ScreenManager::navigateTo("MainMenu");

    // Set up callbacks for Bluetooth manager
    if (coreTaskManager->getBluetoothManager()) {
        coreTaskManager->getBluetoothManager()->setLogCallback([this](const Core::LogPacket& packet, const String& deviceName) {
            onLogReceived(packet, deviceName);
        });

        coreTaskManager->getBluetoothManager()->setConnectionCallback([this](const String& deviceName, bool connected) {
            onDeviceConnection(deviceName, connected);
        });
    }

    initialized = true;
    Serial.println("BTLogger initialized successfully!");

    return true;
}

void BTLoggerApp::start() {
    if (!initialized) {
        if (!initialize()) {
            Serial.println("Failed to initialize BTLogger");
            return;
        }
    }

    running = true;
    Serial.println("BTLogger started - Starting core tasks...");

    // Start the core task manager
    coreTaskManager->start();

    // Start scanning for devices
    if (coreTaskManager->getBluetoothManager()) {
        coreTaskManager->getBluetoothManager()->startScanning();
    }

    // Show welcome toast
    UI::ToastManager::showSuccess("BTLogger Ready - Scanning for devices...");
}

void BTLoggerApp::stop() {
    if (!running) {
        return;
    }

    running = false;
    Serial.println("Stopping BTLogger...");

    if (coreTaskManager) {
        coreTaskManager->stop();
    }

    Serial.println("BTLogger stopped");
}

void BTLoggerApp::update() {
    if (!running || !initialized) {
        return;
    }

    unsigned long currentTime = millis();

    // Very light update loop - UI is handled in UI task, just update LEDs occasionally
    if (currentTime - lastUpdate > 1000) {  // Only update once per second
        // Update LEDs
        updateLEDs();

        lastUpdate = currentTime;
    }

    // Small delay to prevent watchdog issues and reduce CPU usage
    delay(10);
}

void BTLoggerApp::handleInput() {
    // Input handling is done in the UI task
    // This is kept for compatibility but is no longer needed
}

void BTLoggerApp::onLogReceived(const Core::LogPacket& packet, const String& deviceName) {
    // Save to SD card (this callback runs on communications core)
    if (coreTaskManager->getSDCardManager()) {
        coreTaskManager->getSDCardManager()->saveLogToSession(packet, deviceName);
    }

    // Print to serial for debugging
    Serial.printf("[%s] %s: %s\n", deviceName.c_str(), packet.tag, packet.message);

    // Send to LogViewer screen if it exists
    auto screen = UI::ScreenManager::getScreen("LogViewer");
    if (screen) {
        // We know this is a LogViewerScreen based on the name
        auto logViewer = static_cast<UI::Screens::LogViewerScreen*>(screen);
        logViewer->addLogEntry(deviceName, packet.tag, packet.message, packet.level);
    }

    // Send message to UI task for toast notifications
    Core::CoreMessage message(Core::MSG_LOG_RECEIVED, deviceName, String(packet.message), packet.level);
    coreTaskManager->sendToUI(message);
}

void BTLoggerApp::onDeviceConnection(const String& deviceName, bool connected) {
    if (connected) {
        Serial.printf("Device connected: %s\n", deviceName.c_str());

        // Start new logging session
        if (coreTaskManager->getSDCardManager()) {
            coreTaskManager->getSDCardManager()->startNewSession(deviceName);
        }

        // Update status footer with connection info
        UI::ScreenManager::setStatusText("Connected: " + deviceName);
    } else {
        Serial.printf("Device disconnected: %s\n", deviceName.c_str());

        // End current session
        if (coreTaskManager->getSDCardManager()) {
            coreTaskManager->getSDCardManager()->endCurrentSession();
        }

        // Update status footer
        UI::ScreenManager::setStatusText("Disconnected - Scanning for devices...");
    }

    // Send message to UI task for any additional processing if needed
    Core::CoreMessage message(Core::MSG_DEVICE_CONNECTION, deviceName, "", connected ? 1 : 0);
    coreTaskManager->sendToUI(message);
}

void BTLoggerApp::onDeviceConnectRequest(const String& address) {
    if (coreTaskManager->getBluetoothManager()) {
        Serial.printf("Connecting to device: %s\n", address.c_str());
        coreTaskManager->getBluetoothManager()->connectToDevice(address);
    }
}

void BTLoggerApp::onFileOperation(const String& operation, const String& path) {
    // Send file operation to communications task
    Core::CoreMessage message(Core::MSG_FILE_OPERATION, operation, path);
    coreTaskManager->sendToCommunications(message);
}

void BTLoggerApp::setupHardware() {
    // Initialize LEDs
    pinMode(led_pin[0], OUTPUT);
    pinMode(led_pin[1], OUTPUT);
    pinMode(led_pin[2], OUTPUT);

    // Turn off all LEDs initially
    digitalWrite(led_pin[0], HIGH);
    digitalWrite(led_pin[1], HIGH);
    digitalWrite(led_pin[2], HIGH);

    Serial.println("Hardware initialized");
}

void BTLoggerApp::updateLEDs() {
    static unsigned long lastLedUpdate = 0;
    static bool ledState = false;

    unsigned long currentTime = millis();

    if (currentTime - lastLedUpdate > 1000) {  // Update every second
        // LED 0 (Red) - Power/Status
        digitalWrite(led_pin[0], running ? LOW : HIGH);

        // LED 1 (Green) - Bluetooth connection
        bool hasConnection = coreTaskManager &&
                             coreTaskManager->getBluetoothManager() &&
                             coreTaskManager->getBluetoothManager()->getConnectedDeviceCount() > 0;
        digitalWrite(led_pin[1], hasConnection ? LOW : HIGH);

        // LED 2 (Blue) - SD Card activity (blink when logging)
        bool hasSDCard = coreTaskManager &&
                         coreTaskManager->getSDCardManager() &&
                         coreTaskManager->getSDCardManager()->isCardPresent();
        if (hasSDCard && !coreTaskManager->getSDCardManager()->getCurrentSessionFile().isEmpty()) {
            // Blink when actively logging
            digitalWrite(led_pin[2], ledState ? LOW : HIGH);
            ledState = !ledState;
        } else {
            digitalWrite(led_pin[2], hasSDCard ? LOW : HIGH);
        }

        lastLedUpdate = currentTime;
    }
}

}  // namespace BTLogger