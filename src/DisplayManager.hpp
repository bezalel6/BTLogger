#pragma once

#include <Arduino.h>
#include "ESP32_SPI_9341.h"
#include "BluetoothManager.hpp"
#include <vector>

struct LogEntry {
    String deviceName;
    String tag;
    String message;
    uint8_t level;
    unsigned long timestamp;
};

class DisplayManager {
   public:
    DisplayManager(LGFX& display);
    ~DisplayManager();

    bool initialize();
    void update();

    // Display functions
    void showStartupScreen();
    void showMainScreen();
    void addLogEntry(const LogPacket& packet, const String& deviceName);
    void updateConnectionStatus(const String& deviceName, bool connected);
    void updateSDCardStatus(bool present, size_t freeSpace, size_t totalSpace);
    void showMessage(const String& title, const String& message, bool isError = false);
    void clearScreen();

   private:
    LGFX& lcd;
    bool initialized;

    // Partial buffer for log area only (to reduce memory usage)
    LGFX_Sprite* logBuffer;
    bool needsLogRedraw;
    bool needsStatusRedraw;

    // Display state
    std::vector<LogEntry> logEntries;
    String connectedDevice;
    bool sdCardPresent;
    size_t sdFreeSpace;
    size_t sdTotalSpace;

    // Display parameters
    static const int MAX_LOG_ENTRIES = 50;
    static const int LINE_HEIGHT = 16;
    static const int HEADER_HEIGHT = 40;

    // Helper functions
    void drawHeader();
    void drawStatusBar();
    void drawLogEntries();
    String formatFileSize(size_t bytes);
    uint16_t getLogLevelColor(uint8_t level);
    void scrollLogsUp();
    void requestRedraw();
};