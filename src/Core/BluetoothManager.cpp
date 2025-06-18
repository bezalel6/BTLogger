#include "BluetoothManager.hpp"
#include <esp_bt_main.h>
#include <esp_bt_device.h>

namespace BTLogger {
namespace Core {

// Static instance for callbacks
BluetoothManager* BluetoothManager::instance = nullptr;

// BLE Scan Callbacks
class BTLoggerScanCallbacks : public BLEAdvertisedDeviceCallbacks {
   public:
    void onResult(BLEAdvertisedDevice advertisedDevice) override {
        if (BluetoothManager::instance) {
            BluetoothManager::instance->onDeviceFound(advertisedDevice);
        }
    }
};

// BLE Client Callbacks
class BTLoggerClientCallbacks : public BLEClientCallbacks {
   private:
    String deviceAddress;

   public:
    BTLoggerClientCallbacks(const String& address) : deviceAddress(address) {}

    void onConnect(BLEClient* client) override {
        Serial.printf("Connected to device: %s\n", deviceAddress.c_str());
    }

    void onDisconnect(BLEClient* client) override {
        Serial.printf("Disconnected from device: %s\n", deviceAddress.c_str());
        if (BluetoothManager::instance) {
            BluetoothManager::instance->onDeviceDisconnected(deviceAddress);
        }
    }
};

BluetoothManager::BluetoothManager()
    : scanner(nullptr), scanning(false), lastScanTime(0) {
    // Set UUIDs for BTLogger communication
    targetServiceUUID = "12345678-1234-1234-1234-123456789ABC";
    logCharacteristicUUID = "87654321-4321-4321-4321-CBA987654321";

    instance = this;
}

BluetoothManager::~BluetoothManager() {
    stopScanning();

    // Disconnect all devices
    for (auto& device : connectedDevices) {
        if (device.client && device.connected) {
            device.client->disconnect();
            delete device.client;
        }
    }

    connectedDevices.clear();
    instance = nullptr;
}

bool BluetoothManager::initialize() {
    Serial.println("Initializing Bluetooth...");

    // Initialize BLE
    BLEDevice::init("BTLogger");
    BLEDevice::setPower(ESP_PWR_LVL_P7);

    // Create BLE scanner
    scanner = BLEDevice::getScan();
    scanner->setAdvertisedDeviceCallbacks(new BTLoggerScanCallbacks());
    scanner->setActiveScan(true);
    scanner->setInterval(100);
    scanner->setWindow(99);

    Serial.println("Bluetooth initialized successfully");
    return true;
}

void BluetoothManager::startScanning() {
    if (scanning || !scanner) {
        return;
    }

    Serial.println("Starting BLE scan...");
    scanning = true;
    lastScanTime = millis();

    // Clear previous scan results
    availableDevices.clear();

    // Start scan (5 seconds)
    scanner->start(5, &scanCompleteCallback, false);
}

void BluetoothManager::stopScanning() {
    if (!scanning || !scanner) {
        return;
    }

    Serial.println("Stopping BLE scan...");
    scanner->stop();
    scanning = false;
}

void BluetoothManager::scanCompleteCallback(BLEScanResults results) {
    if (BluetoothManager::instance) {
        BluetoothManager::instance->scanning = false;
        Serial.printf("Scan complete. Found %d devices\n", results.getCount());
    }
}

void BluetoothManager::onDeviceFound(BLEAdvertisedDevice advertisedDevice) {
    String deviceName = String(advertisedDevice.getName().c_str());
    String deviceAddress = String(advertisedDevice.getAddress().toString().c_str());

    // Filter devices - look for BTLogger compatible devices
    bool isTargetDevice = false;

    if (deviceName.length() > 0) {
        // Accept devices with these patterns
        if (deviceName.indexOf("BTLogger") >= 0 ||
            deviceName.indexOf("ESP32") >= 0 ||
            deviceName.indexOf("WeatherStation") >= 0 ||
            deviceName.indexOf("MyDevice") >= 0 ||
            deviceName.indexOf("_v") >= 0) {  // Version pattern like "_v2.1"
            isTargetDevice = true;
        }
    }

    if (isTargetDevice) {
        Serial.printf("Found target device: %s (%s)\n", deviceName.c_str(), deviceAddress.c_str());

        // Check if we already have this device
        bool alreadyExists = false;
        for (auto& device : availableDevices) {
            String existingAddress = String(device.getAddress().toString().c_str());
            if (existingAddress.equals(deviceAddress)) {
                alreadyExists = true;
                break;
            }
        }

        if (!alreadyExists) {
            availableDevices.push_back(advertisedDevice);

            // Auto-connect to the first compatible device
            if (getConnectedDeviceCount() == 0) {
                Serial.printf("Auto-connecting to: %s\n", deviceName.c_str());
                connectToDevice(deviceAddress);
            }
        }
    }
}

bool BluetoothManager::connectToDevice(const String& address) {
    // Check if already connected
    for (const auto& device : connectedDevices) {
        if (device.address == address && device.connected) {
            Serial.println("Device already connected");
            return true;
        }
    }

    Serial.printf("Connecting to device: %s\n", address.c_str());

    // Find the advertised device
    BLEAdvertisedDevice* targetDevice = nullptr;
    for (auto& device : availableDevices) {
        String deviceAddress = String(device.getAddress().toString().c_str());
        if (deviceAddress.equals(address)) {
            targetDevice = &device;
            break;
        }
    }

    if (!targetDevice) {
        Serial.println("Device not found in scan results");
        return false;
    }

    // Create BLE client
    BLEClient* client = BLEDevice::createClient();
    client->setClientCallbacks(new BTLoggerClientCallbacks(address));

    // Connect to device
    if (!client->connect(targetDevice)) {
        Serial.println("Failed to connect to device");
        delete client;
        return false;
    }

    Serial.println("Connected! Looking for service...");

    // Get the service
    BLERemoteService* service = client->getService(targetServiceUUID.c_str());
    if (!service) {
        Serial.println("Target service not found");
        client->disconnect();
        delete client;
        return false;
    }

    Serial.println("Service found! Looking for characteristic...");

    // Get the characteristic
    BLERemoteCharacteristic* characteristic = service->getCharacteristic(logCharacteristicUUID.c_str());
    if (!characteristic) {
        Serial.println("Log characteristic not found");
        client->disconnect();
        delete client;
        return false;
    }

    Serial.println("Characteristic found! Setting up notifications...");

    // Register for notifications
    if (characteristic->canNotify()) {
        characteristic->registerForNotify(&notifyCallback);
        Serial.println("Notifications enabled");
    }

    // Add to connected devices
    ConnectedDevice newDevice;
    newDevice.name = String(targetDevice->getName().c_str());
    newDevice.address = address;
    newDevice.client = client;
    newDevice.connected = true;
    newDevice.lastSeen = millis();

    connectedDevices.push_back(newDevice);

    // Call connection callback
    if (connectionCallback) {
        connectionCallback(newDevice.name, true);
    }

    Serial.printf("Successfully connected to: %s\n", newDevice.name.c_str());
    return true;
}

void BluetoothManager::disconnectDevice(const String& address) {
    for (auto it = connectedDevices.begin(); it != connectedDevices.end(); ++it) {
        if (it->address == address) {
            if (it->client && it->connected) {
                it->client->disconnect();
                delete it->client;
            }

            // Call connection callback
            if (connectionCallback) {
                connectionCallback(it->name, false);
            }

            connectedDevices.erase(it);
            Serial.printf("Disconnected from device: %s\n", address.c_str());
            break;
        }
    }
}

void BluetoothManager::onDeviceDisconnected(const String& address) {
    for (auto it = connectedDevices.begin(); it != connectedDevices.end(); ++it) {
        if (it->address == address) {
            it->connected = false;

            // Call connection callback
            if (connectionCallback) {
                connectionCallback(it->name, false);
            }

            // Clean up
            if (it->client) {
                delete it->client;
            }

            connectedDevices.erase(it);
            Serial.printf("Device disconnected: %s\n", address.c_str());
            break;
        }
    }
}

void BluetoothManager::notifyCallback(BLERemoteCharacteristic* characteristic, uint8_t* data, size_t length, bool isNotify) {
    if (BluetoothManager::instance) {
        String deviceAddress = characteristic->getRemoteService()->getClient()->getPeerAddress().toString().c_str();
        BluetoothManager::instance->processIncomingData(deviceAddress, data, length);
    }
}

void BluetoothManager::processIncomingData(const String& deviceAddress, uint8_t* data, size_t length) {
    if (length < sizeof(LogPacket)) {
        Serial.println("Received invalid log packet (too small)");
        return;
    }

    // Cast the data to LogPacket
    LogPacket* packet = reinterpret_cast<LogPacket*>(data);

    // Validate packet
    if (packet->length > sizeof(packet->message) - 1) {
        Serial.println("Received invalid log packet (message too long)");
        return;
    }

    // Ensure null termination
    packet->message[packet->length] = '\0';
    packet->tag[sizeof(packet->tag) - 1] = '\0';

    // Find device name
    String deviceName = "Unknown";
    for (const auto& device : connectedDevices) {
        if (device.address == deviceAddress) {
            deviceName = device.name;
            break;
        }
    }

    // Call log callback
    if (logCallback) {
        logCallback(*packet, deviceName);
    }
}

void BluetoothManager::update() {
    unsigned long currentTime = millis();

    // Restart scanning if needed (every 30 seconds)
    if (!scanning && (currentTime - lastScanTime > 30000)) {
        if (getConnectedDeviceCount() == 0) {
            startScanning();
        }
    }

    // Update device last seen times
    for (auto& device : connectedDevices) {
        if (device.connected) {
            device.lastSeen = currentTime;
        }
    }
}

size_t BluetoothManager::getConnectedDeviceCount() const {
    size_t count = 0;
    for (const auto& device : connectedDevices) {
        if (device.connected) {
            count++;
        }
    }
    return count;
}

std::vector<String> BluetoothManager::getConnectedDeviceNames() const {
    std::vector<String> names;
    for (const auto& device : connectedDevices) {
        if (device.connected) {
            names.push_back(device.name);
        }
    }
    return names;
}

std::vector<String> BluetoothManager::getAvailableDevices() const {
    std::vector<String> devices;
    for (auto& device : const_cast<std::vector<BLEAdvertisedDevice>&>(availableDevices)) {
        String name = String(device.getName().c_str());
        String address = String(device.getAddress().toString().c_str());
        devices.push_back(name + " (" + address + ")");
    }
    return devices;
}

ConnectedDevice* BluetoothManager::findDevice(const String& address) {
    for (auto& device : connectedDevices) {
        if (device.address == address) {
            return &device;
        }
    }
    return nullptr;
}

}  // namespace Core
}  // namespace BTLogger