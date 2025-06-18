#pragma once

// BTLogger Configuration
// Uncomment the line below to use software SPI (bitbang) for touch controller
// This resolves SPI conflicts on ESP32 Cheap Yellow Display boards
#define USE_BITBANG_TOUCH

#include <Arduino.h>
#include "Hardware/ESP32_SPI_9341.h"
#include "Core/CoreTaskManager.hpp"
#include "Core/BluetoothManager.hpp"
#include "Core/SDCardManager.hpp"
#include "UI/ScreenManager.hpp"
#include "UI/TouchManager.hpp"
#include "UI/CriticalErrorHandler.hpp"

namespace BTLogger {

// LED pin definitions
const int led_pin[3] = {4, 16, 17};  // Red, Green, Blue LEDs

/**
 * Main application class that coordinates all subsystems
 */
class BTLoggerApp {
   public:
    BTLoggerApp();
    ~BTLoggerApp();

    // Core lifecycle
    bool initialize();
    void run();
    void start();
    void stop();
    void update();

    // Getters for subsystems
    Core::BluetoothManager& getBluetooth() { return bluetoothManager; }
    Core::SDCardManager& getSDCard() { return sdCardManager; }
    Core::CoreTaskManager* getTaskManager() { return coreTaskManager; }
    UI::ScreenManager& getScreenManager() { return screenManager; }

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

    // Core subsystems
    Core::BluetoothManager bluetoothManager;
    Core::SDCardManager sdCardManager;
    Core::CoreTaskManager* coreTaskManager;

    // UI subsystems
    UI::ScreenManager screenManager;

    // State
    bool running;
    bool initialized;
    unsigned long lastUpdate;

    // Internal methods
    bool initializeDisplay();
    bool initializeCore();
    bool initializeUI();
    void setupHardware();
    void updateLEDs();

    void handleLoop();
    void handleError(const String& error);

    // Event handlers (these will be called from callbacks)
    void onLogReceived(const Core::LogPacket& packet, const String& deviceName);
    void onDeviceConnection(const String& deviceName, bool connected);
};

}  // namespace BTLogger