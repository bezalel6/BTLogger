#pragma once

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEClient.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <functional>
#include <vector>

// Log packet structure for communication
struct LogPacket {
    uint32_t timestamp;
    uint8_t level;  // 0=DEBUG, 1=INFO, 2=WARN, 3=ERROR
    uint16_t length;
    char message[256];
    char tag[32];

    LogPacket() : timestamp(0), level(0), length(0) {
        message[0] = '\0';
        tag[0] = '\0';
    }
};

// Device connection info
struct ConnectedDevice {
    String name;
    String address;
    BLEClient* client;
    bool connected;
    unsigned long lastSeen;

    ConnectedDevice() : client(nullptr), connected(false), lastSeen(0) {}
};

// Callback types
using LogCallback = std::function<void(const LogPacket&, const String& deviceName)>;
using ConnectionCallback = std::function<void(const String& deviceName, bool connected)>;

class BluetoothManager {
   public:
    BluetoothManager();
    ~BluetoothManager();

    // Core functionality
    bool initialize();
    void startScanning();
    void stopScanning();
    bool connectToDevice(const String& address);
    void disconnectDevice(const String& address);
    void update();

    // Callbacks
    void setLogCallback(LogCallback callback) { logCallback = callback; }
    void setConnectionCallback(ConnectionCallback callback) { connectionCallback = callback; }

    // Status
    bool isScanning() const { return scanning; }
    size_t getConnectedDeviceCount() const;
    std::vector<String> getConnectedDeviceNames() const;
    std::vector<String> getAvailableDevices() const;

    // Configuration
    void setTargetServiceUUID(const String& uuid) { targetServiceUUID = uuid; }
    void setLogCharacteristicUUID(const String& uuid) { logCharacteristicUUID = uuid; }

    // Public methods for callbacks (needed by BLE callback classes)
    void onDeviceFound(BLEAdvertisedDevice advertisedDevice);
    void onDeviceDisconnected(const String& address);

    // Static instance for callbacks
    static BluetoothManager* instance;

   private:
    // BLE objects
    BLEScan* scanner;
    std::vector<ConnectedDevice> connectedDevices;
    std::vector<BLEAdvertisedDevice> availableDevices;

    // Configuration
    String targetServiceUUID;
    String logCharacteristicUUID;
    bool scanning;
    unsigned long lastScanTime;

    // Callbacks
    LogCallback logCallback;
    ConnectionCallback connectionCallback;

    // Internal methods
    void onDeviceConnected(const String& address);
    void processIncomingData(const String& deviceAddress, uint8_t* data, size_t length);
    ConnectedDevice* findDevice(const String& address);

    // Static callback wrappers
    static void scanCompleteCallback(BLEScanResults results);
    static void notifyCallback(BLERemoteCharacteristic* characteristic, uint8_t* data, size_t length, bool isNotify);
};