#include "BTLoggerApp.hpp"
#include "UI/UIScale.hpp"
#include "UI/TouchManager.hpp"
#include "UI/ToastManager.hpp"
#include "UI/MenuManager.hpp"
#include "UI/CriticalErrorHandler.hpp"

namespace BTLogger {

BTLoggerApp::BTLoggerApp()
    : bluetoothManager(nullptr), sdCardManager(nullptr), running(false), initialized(false), lastUpdate(0) {
}

BTLoggerApp::~BTLoggerApp() {
    stop();

    delete sdCardManager;
    delete bluetoothManager;
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

    // Initialize managers
    Serial.println("Initializing managers...");

    // SD Card Manager
    sdCardManager = new Core::SDCardManager();
    if (!sdCardManager->initialize()) {
        Serial.println("WARNING: SD Card initialization failed - logging disabled");
    }

    // Bluetooth Manager
    bluetoothManager = new Core::BluetoothManager();
    if (!bluetoothManager->initialize()) {
        Serial.println("ERROR: Bluetooth initialization failed");
        return false;
    }

    // Set up callbacks
    bluetoothManager->setLogCallback([this](const Core::LogPacket& packet, const String& deviceName) {
        onLogReceived(packet, deviceName);
    });

    bluetoothManager->setConnectionCallback([this](const String& deviceName, bool connected) {
        onDeviceConnection(deviceName, connected);
    });

    // Initialize UI systems
    UI::UIScale::initialize();
    if (!UI::TouchManager::initialize(lcd)) {
        Serial.println("ERROR: Touch initialization failed");
        return false;
    }
    UI::ToastManager::initialize(lcd);
    UI::CriticalErrorHandler::initialize(lcd);

    // Wait for touch calibration to complete if needed
    while (UI::TouchManager::needsCalibration()) {
        UI::TouchManager::update();
        delay(50);
    }

    // Initialize menu system
    UI::MenuManager::initialize(lcd);

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
    Serial.println("BTLogger started - Ready to receive logs");

    // Start scanning for devices
    bluetoothManager->startScanning();

    // Show welcome toast
    UI::ToastManager::showSuccess("BTLogger Ready - Scanning for devices...");
}

void BTLoggerApp::stop() {
    if (!running) {
        return;
    }

    running = false;
    Serial.println("Stopping BTLogger...");

    if (bluetoothManager) {
        bluetoothManager->stopScanning();
    }

    if (sdCardManager) {
        sdCardManager->endCurrentSession();
    }

    Serial.println("BTLogger stopped");
}

void BTLoggerApp::update() {
    if (!running || !initialized) {
        return;
    }

    unsigned long currentTime = millis();

    // Update managers (limit frequency to reduce overhead)
    if (currentTime - lastUpdate > 10) {  // 100Hz update rate
        if (bluetoothManager) {
            bluetoothManager->update();
        }

        // Update UI systems
        UI::TouchManager::update();
        UI::ToastManager::update();
        UI::MenuManager::update();

        lastUpdate = currentTime;
    }

    // Update LEDs
    updateLEDs();
}

void BTLoggerApp::handleInput() {
    // Input handling is done through the existing input system
    // Hardware buttons/encoder events are handled in the input callbacks
}

void BTLoggerApp::onLogReceived(const Core::LogPacket& packet, const String& deviceName) {
    // Save to SD card
    if (sdCardManager) {
        sdCardManager->saveLogToSession(packet, deviceName);
    }

    // Print to serial for debugging
    Serial.printf("[%s] %s: %s\n", deviceName.c_str(), packet.tag, packet.message);

    // Show toast notification for important logs
    if (packet.level >= 2) {  // WARN and ERROR levels
        String levelStr = (packet.level == 2) ? "WARN" : "ERROR";
        String toastMsg = deviceName + " " + levelStr + ": " + String(packet.message);
        if (packet.level == 3) {
            UI::ToastManager::showError(toastMsg);
        } else {
            UI::ToastManager::showWarning(toastMsg);
        }
    }
}

void BTLoggerApp::onDeviceConnection(const String& deviceName, bool connected) {
    if (connected) {
        Serial.printf("Device connected: %s\n", deviceName.c_str());

        // Start new logging session
        if (sdCardManager) {
            sdCardManager->startNewSession(deviceName);
        }

        // Show connection toast
        UI::ToastManager::showSuccess("Connected to " + deviceName);
    } else {
        Serial.printf("Device disconnected: %s\n", deviceName.c_str());

        // End current session
        if (sdCardManager) {
            sdCardManager->endCurrentSession();
        }

        // Show disconnection toast
        UI::ToastManager::showWarning("Disconnected from " + deviceName);
    }
}

void BTLoggerApp::onDeviceConnectRequest(const String& address) {
    if (bluetoothManager) {
        Serial.printf("Connecting to device: %s\n", address.c_str());
        bluetoothManager->connectToDevice(address);
    }
}

void BTLoggerApp::onFileOperation(const String& operation, const String& path) {
    if (!sdCardManager) {
        return;
    }

    if (operation == "load") {
        // Load log file
        std::vector<String> lines = sdCardManager->loadLogFile(path);
        if (!lines.empty()) {
            UI::ToastManager::showSuccess("Loaded " + String(lines.size()) + " log entries");
            // File content would be displayed in the log viewer screen
        } else {
            UI::ToastManager::showError("Failed to load file: " + path);
        }
    } else if (operation == "delete") {
        // Delete file
        if (sdCardManager->deleteFile(path)) {
            UI::ToastManager::showSuccess("File deleted: " + path);
        } else {
            UI::ToastManager::showError("Failed to delete: " + path);
        }
    }
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
        bool hasConnection = bluetoothManager && bluetoothManager->getConnectedDeviceCount() > 0;
        digitalWrite(led_pin[1], hasConnection ? LOW : HIGH);

        // LED 2 (Blue) - SD Card activity (blink when logging)
        bool hasSDCard = sdCardManager && sdCardManager->isCardPresent();
        if (hasSDCard && !sdCardManager->getCurrentSessionFile().isEmpty()) {
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