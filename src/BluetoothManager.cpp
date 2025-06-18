#include "BluetoothManager.hpp"

// Static instance for callbacks
BluetoothManager* BluetoothManager::instance = nullptr;

// Default UUIDs for BTLogger service
static const String DEFAULT_SERVICE_UUID = "12345678-1234-1234-1234-123456789abc";
static const String DEFAULT_LOG_CHAR_UUID = "87654321-4321-4321-4321-cba987654321";

// Callback class for device scanning
class BTLoggerScanCallback : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) override {
        if (BluetoothManager::instance) {
            BluetoothManager::instance->onDeviceFound(advertisedDevice);
        }
    }
};

// Callback class for client connections
class BTLoggerClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* client) override {
        Serial.printf("Connected to device: %s\n", client->getPeerAddress().toString().c_str());
    }

    void onDisconnect(BLEClient* client) override {
        Serial.printf("Disconnected from device: %s\n", client->getPeerAddress().toString().c_str());
        if (BluetoothManager::instance) {
            BluetoothManager::instance->onDeviceDisconnected(client->getPeerAddress().toString().c_str());
        }
    }
};

BluetoothManager::BluetoothManager()
    : scanner(nullptr), targetServiceUUID(DEFAULT_SERVICE_UUID), logCharacteristicUUID(DEFAULT_LOG_CHAR_UUID), scanning(false), lastScanTime(0) {
    instance = this;
}

BluetoothManager::~BluetoothManager() {
    // Cleanup connections
    for (auto& device : connectedDevices) {
        if (device.client) {
            device.client->disconnect();
            delete device.client;
        }
    }
    connectedDevices.clear();

    if (scanner) {
        scanner->stop();
    }

    instance = nullptr;
}

bool BluetoothManager::initialize() {
    Serial.println("Initializing Bluetooth Manager...");

    // Initialize BLE
    BLEDevice::init("BTLogger_Central");
    BLEDevice::setPower(ESP_PWR_LVL_P7);

    // Create scanner
    scanner = BLEDevice::getScan();
    scanner->setActiveScan(true);
    scanner->setInterval(100);
    scanner->setWindow(99);

    // Set up scan callback
    scanner->setAdvertisedDeviceCallbacks(new BTLoggerScanCallback());

    Serial.println("Bluetooth Manager initialized successfully");
    return true;
}

void BluetoothManager::startScanning() {
    if (!scanner || scanning) return;

    Serial.println("Starting BLE scan...");
    availableDevices.clear();
    scanning = true;
    scanner->start(5, false);  // Scan for 5 seconds, don't clear results
    lastScanTime = millis();
}

void BluetoothManager::stopScanning() {
    if (!scanner || !scanning) return;

    Serial.println("Stopping BLE scan...");
    scanner->stop();
    scanning = false;
}

bool BluetoothManager::connectToDevice(const String& address) {
    Serial.printf("Attempting to connect to device: %s\n", address.c_str());

    // Check if already connected
    ConnectedDevice* existing = findDevice(address);
    if (existing && existing->connected) {
        Serial.println("Device already connected");
        return true;
    }

    // Find the advertised device
    BLEAdvertisedDevice* targetDevice = nullptr;
    for (auto& device : availableDevices) {
        String deviceAddress = device.getAddress().toString().c_str();
        if (deviceAddress == address) {
            targetDevice = &device;
            break;
        }
    }

    if (!targetDevice) {
        Serial.println("Device not found in scan results");
        return false;
    }

    // Create client
    BLEClient* client = BLEDevice::createClient();
    if (!client) {
        Serial.println("Failed to create BLE client");
        return false;
    }

    // Set up client callbacks
    client->setClientCallbacks(new BTLoggerClientCallback());

    // Connect to device
    if (!client->connect(targetDevice)) {
        Serial.println("Failed to connect to device");
        delete client;
        return false;
    }

    // Get service
    BLERemoteService* service = client->getService(targetServiceUUID.c_str());
    if (!service) {
        Serial.println("Failed to find target service");
        client->disconnect();
        delete client;
        return false;
    }

    // Get characteristic
    BLERemoteCharacteristic* characteristic = service->getCharacteristic(logCharacteristicUUID.c_str());
    if (!characteristic) {
        Serial.println("Failed to find log characteristic");
        client->disconnect();
        delete client;
        return false;
    }

    // Set up notifications
    if (characteristic->canNotify()) {
        characteristic->registerForNotify([](BLERemoteCharacteristic* characteristic, uint8_t* data, size_t length, bool isNotify) {
            if (BluetoothManager::instance) {
                BluetoothManager::instance->processIncomingData(
                    characteristic->getRemoteService()->getClient()->getPeerAddress().toString().c_str(),
                    data,
                    length);
            }
        });
    }

    // Add to connected devices
    ConnectedDevice newDevice;
    newDevice.name = targetDevice->getName().c_str();
    newDevice.address = address;
    newDevice.client = client;
    newDevice.connected = true;
    newDevice.lastSeen = millis();

    if (existing) {
        *existing = newDevice;
    } else {
        connectedDevices.push_back(newDevice);
    }

    onDeviceConnected(address);
    return true;
}

void BluetoothManager::disconnectDevice(const String& address) {
    ConnectedDevice* device = findDevice(address);
    if (!device || !device->connected) {
        return;
    }

    Serial.printf("Disconnecting device: %s\n", address.c_str());

    if (device->client) {
        device->client->disconnect();
        delete device->client;
        device->client = nullptr;
    }

    device->connected = false;
    onDeviceDisconnected(address);
}

void BluetoothManager::update() {
    unsigned long currentTime = millis();

    // Check connection health
    for (auto& device : connectedDevices) {
        if (device.connected && device.client) {
            if (!device.client->isConnected()) {
                device.connected = false;
                onDeviceDisconnected(device.address);
            } else {
                device.lastSeen = currentTime;
            }
        }
    }

    // Auto-connect to discovered devices if we have none connected
    if (getConnectedDeviceCount() == 0 && !availableDevices.empty()) {
        static unsigned long lastConnectionAttempt = 0;
        if (currentTime - lastConnectionAttempt > 3000) {  // Try every 3 seconds
            // Try to connect to the first available device
            auto& device = availableDevices[0];
            String address = device.getAddress().toString().c_str();
            String name = device.getName().c_str();

            Serial.printf("Attempting auto-connection to %s (%s)...\n", name.c_str(), address.c_str());
            if (connectToDevice(address)) {
                Serial.println("Auto-connection successful!");
            } else {
                Serial.println("Auto-connection failed, will retry...");
            }
            lastConnectionAttempt = currentTime;
        }
    }

    // Auto-restart scanning if needed
    if (!scanning && (currentTime - lastScanTime > 10000)) {  // Restart every 10 seconds
        startScanning();
    }
}

size_t BluetoothManager::getConnectedDeviceCount() const {
    size_t count = 0;
    for (const auto& device : connectedDevices) {
        if (device.connected) count++;
    }
    return count;
}

std::vector<String> BluetoothManager::getConnectedDeviceNames() const {
    std::vector<String> names;
    for (const auto& device : connectedDevices) {
        if (device.connected) {
            names.push_back(device.name.isEmpty() ? device.address : device.name);
        }
    }
    return names;
}

std::vector<String> BluetoothManager::getAvailableDevices() const {
    std::vector<String> devices;
    for (const auto& device : availableDevices) {
        // Use const_cast to call non-const methods (BLE library limitation)
        auto& mutableDevice = const_cast<BLEAdvertisedDevice&>(device);
        String name = mutableDevice.getName().c_str();
        String address = mutableDevice.getAddress().toString().c_str();
        devices.push_back(name.isEmpty() ? address : name + " (" + address + ")");
    }
    return devices;
}

void BluetoothManager::onDeviceFound(BLEAdvertisedDevice advertisedDevice) {
    String deviceName = advertisedDevice.getName().c_str();
    String deviceAddress = advertisedDevice.getAddress().toString().c_str();

    // Log all discovered devices for debugging
    Serial.printf("Discovered device: %s (%s) RSSI: %d\n",
                  deviceName.c_str(), deviceAddress.c_str(), advertisedDevice.getRSSI());

    // Check if device advertises our service or has "BTLogger" in name
    bool isTarget = false;
    bool hasService = false;

    if (advertisedDevice.haveServiceUUID()) {
        if (advertisedDevice.isAdvertisingService(BLEUUID(targetServiceUUID.c_str()))) {
            isTarget = true;
            hasService = true;
            Serial.printf("  -> Device advertises BTLogger service UUID\n");
        }
    }

    // Accept devices with specific names that might be BTLogger senders
    if (deviceName.indexOf("BTLogger") >= 0 ||
        deviceName.indexOf("ESP32") >= 0 ||
        deviceName.indexOf("WeatherStation") >= 0 ||
        deviceName.indexOf("MyDevice") >= 0 ||
        deviceName.indexOf("_v") >= 0) {  // Accept devices with version numbers (e.g., "Something_v1.2")
        isTarget = true;
        Serial.printf("  -> Device name matches BTLogger pattern\n");
    }

    if (isTarget) {
        // Add to available devices if not already present
        bool found = false;
        for (const auto& device : availableDevices) {
            // Use const_cast to call non-const methods (BLE library limitation)
            auto& mutableDevice = const_cast<BLEAdvertisedDevice&>(device);
            String existingAddress = mutableDevice.getAddress().toString().c_str();
            if (existingAddress == deviceAddress) {
                found = true;
                break;
            }
        }

        if (!found) {
            availableDevices.push_back(advertisedDevice);
            Serial.printf("*** Found BTLogger target device: %s (%s) ***\n",
                          deviceName.c_str(), deviceAddress.c_str());

            // Auto-connect to the first discovered BTLogger device
            if (getConnectedDeviceCount() == 0) {
                Serial.printf("Auto-connecting to %s...\n", deviceName.c_str());
                // We'll connect in the next update cycle to avoid blocking the scan
            }
        }
    } else {
        Serial.printf("  -> Device ignored (not a BTLogger target)\n");
    }
}

void BluetoothManager::onDeviceConnected(const String& address) {
    ConnectedDevice* device = findDevice(address);
    String deviceName = device ? device->name : address;

    Serial.printf("Device connected: %s\n", deviceName.c_str());

    if (connectionCallback) {
        connectionCallback(deviceName, true);
    }
}

void BluetoothManager::onDeviceDisconnected(const String& address) {
    ConnectedDevice* device = findDevice(address);
    String deviceName = device ? device->name : address;

    Serial.printf("Device disconnected: %s\n", deviceName.c_str());

    if (connectionCallback) {
        connectionCallback(deviceName, false);
    }
}

void BluetoothManager::processIncomingData(const String& deviceAddress, uint8_t* data, size_t length) {
    if (!data || length == 0) return;

    ConnectedDevice* device = findDevice(deviceAddress);
    String deviceName = device ? device->name : deviceAddress;

    // Parse log packet
    LogPacket packet;

    if (length >= sizeof(LogPacket)) {
        // Binary format
        memcpy(&packet, data, sizeof(LogPacket));
    } else {
        // Text format - simple parsing
        String message = String((char*)data, length);
        packet.timestamp = millis();
        packet.level = 1;  // Default to INFO
        packet.length = min((size_t)255, message.length());
        strlcpy(packet.message, message.c_str(), sizeof(packet.message));
        strlcpy(packet.tag, "REMOTE", sizeof(packet.tag));
    }

    Serial.printf("Received log from %s: [%s] %s\n",
                  deviceName.c_str(),
                  packet.tag,
                  packet.message);

    if (logCallback) {
        logCallback(packet, deviceName);
    }
}

ConnectedDevice* BluetoothManager::findDevice(const String& address) {
    for (auto& device : connectedDevices) {
        if (device.address == address) {
            return &device;
        }
    }
    return nullptr;
}