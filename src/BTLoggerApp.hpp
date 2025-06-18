#pragma once

#include <Arduino.h>
#include "ESP32_SPI_9341.h"
#include "BluetoothManager.hpp"
#include "SDCardManager.hpp"
#include "DisplayManager.hpp"

class BTLoggerApp {
   public:
    BTLoggerApp();
    ~BTLoggerApp();

    // Core functionality
    bool initialize();
    void update();
    void handleInput();

    // Application lifecycle
    void start();
    void stop();
    bool isRunning() const { return running; }

   private:
    // Hardware
    LGFX lcd;

    // Managers
    BluetoothManager* bluetoothManager;
    SDCardManager* sdCardManager;
    DisplayManager* displayManager;

    // State
    bool running;
    bool initialized;
    unsigned long lastUpdate;

    // Event handlers
    void onLogReceived(const LogPacket& packet, const String& deviceName);
    void onDeviceConnection(const String& deviceName, bool connected);
    void onDeviceConnectRequest(const String& address);
    void onFileOperation(const String& operation, const String& path);

    // Utility
    void setupHardware();
    void updateLEDs();

    // LED pins from original code
    int led_pin[3] = {17, 4, 16};
};