#include "BTLoggerApp.hpp"
#include "InputSystem.hpp"

BTLoggerApp::BTLoggerApp()
    : bluetoothManager(nullptr), sdCardManager(nullptr), displayManager(nullptr), running(false), initialized(false), lastUpdate(0) {
}

BTLoggerApp::~BTLoggerApp() {
    stop();

    delete displayManager;
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
    sdCardManager = new SDCardManager();
    if (!sdCardManager->initialize()) {
        Serial.println("WARNING: SD Card initialization failed - logging disabled");
    }

    // Bluetooth Manager
    bluetoothManager = new BluetoothManager();
    if (!bluetoothManager->initialize()) {
        Serial.println("ERROR: Bluetooth initialization failed");
        return false;
    }

    // Display Manager
    displayManager = new DisplayManager(lcd);
    if (!displayManager->initialize()) {
        Serial.println("ERROR: Display initialization failed");
        return false;
    }

    // Set up callbacks
    bluetoothManager->setLogCallback([this](const LogPacket& packet, const String& deviceName) {
        onLogReceived(packet, deviceName);
    });

    bluetoothManager->setConnectionCallback([this](const String& deviceName, bool connected) {
        onDeviceConnection(deviceName, connected);
    });

    // Note: DisplayManager doesn't need callbacks since it's simpler
    // Device connection and file operations are handled directly

    // Initialize input system
    INPUT_SETUP();

    // Update display with initial status
    if (sdCardManager && displayManager) {
        displayManager->updateSDCardStatus(
            sdCardManager->isCardPresent(),
            sdCardManager->getFreeSpace(),
            sdCardManager->getTotalSpace());
    }

    // Show startup screen
    if (displayManager) {
        displayManager->showStartupScreen();
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
    Serial.println("BTLogger started - Ready to receive logs");

    // Start scanning for devices
    bluetoothManager->startScanning();

    // Show main screen and welcome message
    if (displayManager) {
        displayManager->showMainScreen();
        displayManager->showMessage("BTLogger Ready", "Scanning for devices...", false);
    }
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

        if (displayManager) {
            displayManager->update();
        }

        lastUpdate = currentTime;
    }

    // Update input system
    INPUT_LOOP();

    // Update LEDs
    updateLEDs();
}

void BTLoggerApp::handleInput() {
    // Input handling is done through the existing input system
    // Hardware buttons/encoder events are handled in the input callbacks
}

void BTLoggerApp::onLogReceived(const LogPacket& packet, const String& deviceName) {
    // Display log on screen
    if (displayManager) {
        displayManager->addLogEntry(packet, deviceName);
    }

    // Save to SD card
    if (sdCardManager) {
        sdCardManager->saveLogToSession(packet, deviceName);
    }

    // Print to serial for debugging
    Serial.printf("[%s] %s: %s\n", deviceName.c_str(), packet.tag, packet.message);
}

void BTLoggerApp::onDeviceConnection(const String& deviceName, bool connected) {
    if (displayManager) {
        displayManager->updateConnectionStatus(deviceName, connected);
    }

    if (connected) {
        Serial.printf("Device connected: %s\n", deviceName.c_str());

        // Start new logging session
        if (sdCardManager) {
            sdCardManager->startNewSession(deviceName);
        }

        // Show connection message
        if (displayManager) {
            displayManager->showMessage("Device Connected", deviceName, false);
        }
    } else {
        Serial.printf("Device disconnected: %s\n", deviceName.c_str());

        // End current session
        if (sdCardManager) {
            sdCardManager->endCurrentSession();
        }
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
        if (displayManager) {
            // Clear current display and show loaded logs
            displayManager->clearScreen();
            displayManager->showMainScreen();

            for (const String& line : lines) {
                // Parse log line and add to display
                // For now, just add as raw text
                LogPacket packet;
                packet.timestamp = millis();
                packet.level = 1;
                strlcpy(packet.message, line.c_str(), sizeof(packet.message));
                strlcpy(packet.tag, "FILE", sizeof(packet.tag));

                displayManager->addLogEntry(packet, "Loaded");
            }
        }
    } else if (operation == "delete") {
        // Delete file
        if (sdCardManager->deleteFile(path)) {
            if (displayManager) {
                displayManager->showMessage("File Deleted", path, false);
            }
        } else {
            if (displayManager) {
                displayManager->showMessage("Delete Failed", path, true);
            }
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