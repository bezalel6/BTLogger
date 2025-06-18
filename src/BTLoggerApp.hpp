#pragma once

#include <Arduino.h>
#include "Hardware/ESP32_SPI_9341.h"
#include "Core/BluetoothManager.hpp"
#include "Core/SDCardManager.hpp"

namespace BTLogger {

// LED pin definitions
const int led_pin[3] = {4, 16, 17};  // Red, Green, Blue LEDs

class BTLoggerApp {
   public:
    BTLoggerApp();
    ~BTLoggerApp();

    // Core lifecycle
    bool initialize();
    void start();
    void stop();
    void update();

    // Input handling (for hardware buttons if needed)
    void handleInput();

    // Status
    bool isRunning() const { return running; }
    bool isInitialized() const { return initialized; }

    // Callbacks for UI integration
    void onDeviceConnectRequest(const String& address);
    void onFileOperation(const String& operation, const String& path);

   private:
    // Hardware
    Hardware::LGFX lcd;

    // Managers
    Core::BluetoothManager* bluetoothManager;
    Core::SDCardManager* sdCardManager;

    // State
    bool running;
    bool initialized;
    unsigned long lastUpdate;

    // Internal methods
    void setupHardware();
    void updateLEDs();

    // Event handlers
    void onLogReceived(const Core::LogPacket& packet, const String& deviceName);
    void onDeviceConnection(const String& deviceName, bool connected);
};

}  // namespace BTLogger