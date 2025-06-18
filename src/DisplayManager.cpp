#include "DisplayManager.hpp"
#include <algorithm>

// LovyanGFX color definitions (RGB565 format)
#define BLACK 0x0000
#define WHITE 0xFFFF
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x001F
#define CYAN 0x07FF
#define YELLOW 0xFFE0
#define GRAY 0x8410
#define NAVY 0x000F

DisplayManager::DisplayManager(LGFX& display)
    : lcd(display), initialized(false), logBuffer(nullptr), needsLogRedraw(true), needsStatusRedraw(true), connectedDevice(""), sdCardPresent(false), sdFreeSpace(0), sdTotalSpace(0) {
}

DisplayManager::~DisplayManager() {
    if (logBuffer) {
        delete logBuffer;
    }
}

bool DisplayManager::initialize() {
    if (initialized) {
        return true;
    }

    // Initialize the display
    lcd.init();
    lcd.setRotation(0);  // Portrait mode
    lcd.setBrightness(200);

    // Create a smaller buffer just for the log area to reduce memory usage
    // Log area dimensions: full width, but limited height
    int logAreaHeight = lcd.height() - (HEADER_HEIGHT + 40);  // Available space for logs
    int bufferHeight = min(logAreaHeight, 160);               // Limit buffer to 160 pixels max

    logBuffer = new LGFX_Sprite(&lcd);
    if (!logBuffer->createSprite(lcd.width(), bufferHeight)) {
        Serial.printf("ERROR: Failed to create log buffer (%dx%d, %d bytes)\n",
                      lcd.width(), bufferHeight, lcd.width() * bufferHeight * 2);
        delete logBuffer;
        logBuffer = nullptr;

        // Fall back to direct rendering without buffer
        Serial.println("WARNING: Using direct rendering (no buffer) - may have flicker");
    } else {
        Serial.printf("Log buffer created: %dx%d (%d bytes)\n",
                      lcd.width(), bufferHeight, lcd.width() * bufferHeight * 2);
    }

    // Clear the screen
    lcd.fillScreen(BLACK);

    initialized = true;
    needsLogRedraw = true;
    needsStatusRedraw = true;
    return true;
}

void DisplayManager::update() {
    if (!initialized) {
        return;
    }

    // Update status bar if needed (direct rendering)
    if (needsStatusRedraw) {
        drawHeader();
        drawStatusBar();
        needsStatusRedraw = false;
    }

    // Update log area (buffered if available, direct otherwise)
    if (needsLogRedraw) {
        drawLogEntries();
        needsLogRedraw = false;
    }
}

void DisplayManager::showStartupScreen() {
    if (!initialized) {
        return;
    }

    lcd.fillScreen(BLACK);
    lcd.setTextColor(WHITE);
    lcd.setTextSize(2);

    int centerX = lcd.width() / 2;
    int centerY = lcd.height() / 2;

    lcd.drawCentreString("BTLogger", centerX, centerY - 40, 1);
    lcd.setTextSize(1);
    lcd.drawCentreString("Bluetooth Log Receiver", centerX, centerY - 10, 1);
    lcd.drawCentreString("Initializing...", centerX, centerY + 20, 1);
}

void DisplayManager::showMainScreen() {
    if (!initialized) {
        return;
    }

    lcd.fillScreen(BLACK);
    needsStatusRedraw = true;
    needsLogRedraw = true;
}

void DisplayManager::addLogEntry(const LogPacket& packet, const String& deviceName) {
    if (!initialized) {
        return;
    }

    LogEntry entry;
    entry.deviceName = deviceName;
    entry.tag = String(packet.tag);
    entry.message = String(packet.message);
    entry.level = packet.level;
    entry.timestamp = packet.timestamp;

    logEntries.push_back(entry);

    // Keep only the most recent entries
    if (logEntries.size() > MAX_LOG_ENTRIES) {
        logEntries.erase(logEntries.begin());
    }

    needsLogRedraw = true;
}

void DisplayManager::updateConnectionStatus(const String& deviceName, bool connected) {
    String previousDevice = connectedDevice;

    if (connected) {
        connectedDevice = deviceName;
    } else {
        connectedDevice = "";
    }

    // Only redraw if connection status actually changed
    if (previousDevice != connectedDevice) {
        needsStatusRedraw = true;
    }
}

void DisplayManager::updateSDCardStatus(bool present, size_t freeSpace, size_t totalSpace) {
    bool changed = (sdCardPresent != present) || (sdFreeSpace != freeSpace) || (sdTotalSpace != totalSpace);

    sdCardPresent = present;
    sdFreeSpace = freeSpace;
    sdTotalSpace = totalSpace;

    if (changed) {
        needsStatusRedraw = true;
    }
}

void DisplayManager::showMessage(const String& title, const String& message, bool isError) {
    if (!initialized) {
        return;
    }

    // Draw current screen first
    drawHeader();
    drawStatusBar();
    drawLogEntries();

    // Draw a message box overlay on top (direct to LCD)
    int boxWidth = 200;
    int boxHeight = 100;
    int x = (lcd.width() - boxWidth) / 2;
    int y = (lcd.height() - boxHeight) / 2;

    lcd.fillRect(x, y, boxWidth, boxHeight, isError ? RED : BLUE);
    lcd.drawRect(x, y, boxWidth, boxHeight, WHITE);

    lcd.setTextColor(WHITE);
    lcd.setTextSize(1);
    lcd.drawCentreString(title.c_str(), lcd.width() / 2, y + 20, 1);
    lcd.drawCentreString(message.c_str(), lcd.width() / 2, y + 40, 1);

    // Show for 2 seconds
    delay(2000);

    // Redraw the screen without the message
    needsStatusRedraw = true;
    needsLogRedraw = true;
}

void DisplayManager::clearScreen() {
    if (!initialized) {
        return;
    }

    lcd.fillScreen(BLACK);
}

void DisplayManager::drawHeader() {
    // Draw title bar directly to LCD
    lcd.fillRect(0, 0, lcd.width(), HEADER_HEIGHT, NAVY);
    lcd.setTextColor(WHITE);
    lcd.setTextSize(2);
    lcd.drawString("BTLogger", 5, 10, 1);

    // Draw separator line
    lcd.drawLine(0, HEADER_HEIGHT, lcd.width(), HEADER_HEIGHT, WHITE);
}

void DisplayManager::drawStatusBar() {
    int statusY = HEADER_HEIGHT + 5;
    lcd.fillRect(0, statusY, lcd.width(), 30, BLACK);

    lcd.setTextColor(WHITE);
    lcd.setTextSize(1);

    // Connection status
    if (!connectedDevice.isEmpty()) {
        lcd.setTextColor(GREEN);
        lcd.drawString("Connected: " + connectedDevice, 5, statusY, 1);
    } else {
        lcd.setTextColor(YELLOW);
        lcd.drawString("Scanning...", 5, statusY, 1);
    }

    // SD Card status
    if (sdCardPresent) {
        lcd.setTextColor(GREEN);
        String sdInfo = "SD: " + formatFileSize(sdFreeSpace) + " free";
        lcd.drawString(sdInfo, 5, statusY + 15, 1);
    } else {
        lcd.setTextColor(RED);
        lcd.drawString("No SD Card", 5, statusY + 15, 1);
    }
}

void DisplayManager::drawLogEntries() {
    int logStartY = HEADER_HEIGHT + 40;
    int availableHeight = lcd.height() - logStartY;
    int maxLines = availableHeight / LINE_HEIGHT;

    if (logBuffer) {
        // Use buffered rendering for smoother updates
        logBuffer->fillScreen(BLACK);

        if (logEntries.empty()) {
            logBuffer->setTextColor(GRAY);
            logBuffer->setTextSize(1);
            logBuffer->drawString("No logs received yet...", 5, 20, 1);
        } else {
            // Draw the most recent log entries to buffer
            int startIndex = std::max(0, (int)logEntries.size() - maxLines);
            int y = 0;  // Start from top of buffer

            for (int i = startIndex; i < logEntries.size() && y < logBuffer->height() - LINE_HEIGHT; i++) {
                const LogEntry& entry = logEntries[i];

                logBuffer->setTextColor(getLogLevelColor(entry.level));
                logBuffer->setTextSize(1);

                // Format: [TAG] Message
                String logLine = "[" + entry.tag + "] " + entry.message;
                if (logLine.length() > 35) {  // Truncate long messages
                    logLine = logLine.substring(0, 32) + "...";
                }

                logBuffer->drawString(logLine, 5, y, 1);
                y += LINE_HEIGHT;
            }
        }

        // Push buffer to screen
        logBuffer->pushSprite(0, logStartY);
    } else {
        // Fall back to direct rendering
        lcd.fillRect(0, logStartY, lcd.width(), availableHeight, BLACK);

        if (logEntries.empty()) {
            lcd.setTextColor(GRAY);
            lcd.setTextSize(1);
            lcd.drawString("No logs received yet...", 5, logStartY + 20, 1);
            return;
        }

        // Draw the most recent log entries directly
        int startIndex = std::max(0, (int)logEntries.size() - maxLines);
        int y = logStartY;

        for (int i = startIndex; i < logEntries.size() && y < lcd.height() - LINE_HEIGHT; i++) {
            const LogEntry& entry = logEntries[i];

            lcd.setTextColor(getLogLevelColor(entry.level));
            lcd.setTextSize(1);

            // Format: [TAG] Message
            String logLine = "[" + entry.tag + "] " + entry.message;
            if (logLine.length() > 35) {  // Truncate long messages
                logLine = logLine.substring(0, 32) + "...";
            }

            lcd.drawString(logLine, 5, y, 1);
            y += LINE_HEIGHT;
        }
    }
}

String DisplayManager::formatFileSize(size_t bytes) {
    if (bytes < 1024) {
        return String(bytes) + "B";
    } else if (bytes < 1024 * 1024) {
        return String(bytes / 1024) + "KB";
    } else {
        return String(bytes / (1024 * 1024)) + "MB";
    }
}

uint16_t DisplayManager::getLogLevelColor(uint8_t level) {
    switch (level) {
        case 1:  // ERROR
            return RED;
        case 2:  // WARNING
            return YELLOW;
        case 3:  // INFO
            return GREEN;
        case 4:  // DEBUG
            return CYAN;
        default:
            return WHITE;
    }
}

void DisplayManager::scrollLogsUp() {
    // This could be implemented for manual scrolling if needed
    // For now, we automatically show the most recent entries
}

void DisplayManager::requestRedraw() {
    needsStatusRedraw = true;
    needsLogRedraw = true;
}