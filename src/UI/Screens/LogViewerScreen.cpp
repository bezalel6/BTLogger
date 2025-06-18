#include "LogViewerScreen.hpp"
#include "../UIScale.hpp"
#include "../TouchManager.hpp"
#include "../ScreenManager.hpp"

namespace BTLogger {
namespace UI {
namespace Screens {

LogViewerScreen::LogViewerScreen() : Screen("LogViewer"),
                                     backButton(nullptr),
                                     clearButton(nullptr),
                                     pauseButton(nullptr),
                                     scrollOffset(0),
                                     maxVisibleLines(0),
                                     paused(false),
                                     lastTouchState(false) {
}

LogViewerScreen::~LogViewerScreen() {
    cleanup();
}

void LogViewerScreen::activate() {
    Screen::activate();

    if (!backButton) {
        // Create control buttons - larger and moved away from edge
        int buttonHeight = UIScale::scale(35);
        int buttonY = UIScale::scale(15);

        backButton = new Widgets::Button(*lcd, UIScale::scale(10), buttonY,
                                         UIScale::scale(BACK_BUTTON_WIDTH), buttonHeight, "BACK");
        backButton->setCallback([this]() {
            Serial.println("Back button pressed in LogViewer");
            goBack();
        });

        clearButton = new Widgets::Button(*lcd, UIScale::scale(75), buttonY,
                                          UIScale::scale(50), buttonHeight, "CLEAR");
        clearButton->setCallback([this]() {
            clearLogs();
        });

        pauseButton = new Widgets::Button(*lcd, UIScale::scale(135), buttonY,
                                          UIScale::scale(50), buttonHeight, "PAUSE");
        pauseButton->setCallback([this]() {
            paused = !paused;
            pauseButton->setText(paused ? "RESUME" : "PAUSE");
            ScreenManager::setStatusText(paused ? "Logging paused" : "Logging resumed");
        });
    }

    // Calculate visible lines
    int contentHeight = lcd->height() - HEADER_HEIGHT - FOOTER_HEIGHT;
    maxVisibleLines = contentHeight / UIScale::scale(LINE_HEIGHT);

    ScreenManager::setStatusText("Log Viewer - Real-time logs");
}

void LogViewerScreen::deactivate() {
    Screen::deactivate();
}

void LogViewerScreen::update() {
    if (!active) return;

    if (needsRedraw) {
        drawHeader();
        drawLogs();
        needsRedraw = false;
    }

    // Update buttons
    if (backButton) backButton->update();
    if (clearButton) clearButton->update();
    if (pauseButton) pauseButton->update();
}

void LogViewerScreen::handleTouch(int x, int y, bool touched) {
    if (!active) return;

    bool wasTapped = TouchManager::wasTapped();

    // Handle scrolling
    if (wasTapped) {
        handleScrolling(x, y, wasTapped);
    }

    if (touched || lastTouchState) {
        // Handle button touches
        if (backButton) backButton->handleTouch(x, y, touched);
        if (clearButton) clearButton->handleTouch(x, y, touched);
        if (pauseButton) pauseButton->handleTouch(x, y, touched);

        lastTouchState = touched;
    }
}

void LogViewerScreen::cleanup() {
    delete backButton;
    delete clearButton;
    delete pauseButton;

    backButton = nullptr;
    clearButton = nullptr;
    pauseButton = nullptr;

    logEntries.clear();
}

void LogViewerScreen::addLogEntry(const String& deviceName, const String& tag, const String& message, int level) {
    // Add new log entry
    logEntries.emplace_back(deviceName, tag, message, level);

    // Limit log entries to prevent memory issues
    if (logEntries.size() > MAX_LOG_ENTRIES) {
        logEntries.erase(logEntries.begin());
        // Adjust scroll offset if needed
        if (scrollOffset > 0) {
            scrollOffset--;
        }
    }

    // Auto-scroll to bottom if not paused and not manually scrolled
    if (!paused && scrollOffset >= (int)logEntries.size() - maxVisibleLines) {
        scrollOffset = std::max(0, (int)logEntries.size() - maxVisibleLines);
    }

    markForRedraw();
}

void LogViewerScreen::clearLogs() {
    logEntries.clear();
    scrollOffset = 0;
    markForRedraw();
}

void LogViewerScreen::drawHeader() {
    if (!lcd) return;

    // Clear header area
    lcd->fillRect(0, 0, lcd->width(), HEADER_HEIGHT, 0x0000);

    // Draw buttons
    if (backButton) backButton->draw();
    if (clearButton) clearButton->draw();
    if (pauseButton) pauseButton->draw();

    // Draw title
    lcd->setTextColor(0x07FF);  // Cyan
    lcd->setTextSize(UIScale::scale(2));
    lcd->setCursor(UIScale::scale(195), UIScale::scale(22));
    lcd->print("LOGS");

    // Draw separator line
    lcd->drawFastHLine(0, HEADER_HEIGHT - 1, lcd->width(), 0x8410);
}

void LogViewerScreen::drawLogs() {
    if (!lcd) return;

    int logAreaY = HEADER_HEIGHT;
    int logAreaHeight = lcd->height() - HEADER_HEIGHT - FOOTER_HEIGHT;

    // Clear log area
    lcd->fillRect(0, logAreaY, lcd->width(), logAreaHeight, 0x0000);

    if (logEntries.empty()) {
        lcd->setTextColor(0x8410);  // Gray
        lcd->setTextSize(UIScale::scale(1));
        lcd->setCursor(UIScale::scale(10), logAreaY + UIScale::scale(20));
        lcd->print("No log entries");
        lcd->setCursor(UIScale::scale(10), logAreaY + UIScale::scale(40));
        lcd->print("Waiting for devices...");
        return;
    }

    // Calculate visible entries
    maxVisibleLines = logAreaHeight / UIScale::scale(LINE_HEIGHT);

    int startIndex = scrollOffset;
    int endIndex = std::min((int)logEntries.size(), startIndex + maxVisibleLines);

    // Draw log entries
    for (int i = startIndex; i < endIndex; i++) {
        const LogEntry& entry = logEntries[i];
        int yPos = logAreaY + ((i - startIndex) * UIScale::scale(LINE_HEIGHT));

        // Format: [LEVEL] Device: Tag: Message
        String levelStr = getLevelString(entry.level);
        uint16_t levelColor = getLevelColor(entry.level);

        lcd->setTextSize(UIScale::scale(1));
        int xPos = UIScale::scale(2);

        // Draw level indicator
        lcd->setTextColor(levelColor);
        lcd->setCursor(xPos, yPos);
        lcd->print("[" + levelStr + "]");
        xPos += UIScale::scale(35);

        // Draw device name (abbreviated if too long)
        lcd->setTextColor(0x07FF);  // Cyan for device name
        lcd->setCursor(xPos, yPos);
        String deviceName = entry.deviceName;
        if (deviceName.length() > 8) {
            deviceName = deviceName.substring(0, 7) + "~";
        }
        lcd->print(deviceName + ":");
        xPos += UIScale::scale(60);

        // Draw tag (abbreviated if needed)
        lcd->setTextColor(0xFFE0);  // Yellow for tag
        lcd->setCursor(xPos, yPos);
        String tag = entry.tag;
        if (tag.length() > 6) {
            tag = tag.substring(0, 5) + "~";
        }
        lcd->print(tag + ":");
        xPos += UIScale::scale(45);

        // Draw message (truncated to fit)
        lcd->setTextColor(0xFFFF);  // White for message
        lcd->setCursor(xPos, yPos);
        String message = entry.message;
        int remainingWidth = lcd->width() - xPos - UIScale::scale(5);
        int maxChars = remainingWidth / UIScale::scale(6);  // Approximate character width
        if (message.length() > maxChars) {
            message = message.substring(0, maxChars - 1) + "~";
        }
        lcd->print(message);
    }

    // Draw scroll indicators
    if (logEntries.size() > maxVisibleLines) {
        int indicatorX = lcd->width() - UIScale::scale(8);

        if (scrollOffset > 0) {
            lcd->setTextColor(0xFFFF);
            lcd->setTextSize(1);
            lcd->setCursor(indicatorX, logAreaY + UIScale::scale(2));
            lcd->print("^");
        }

        if (scrollOffset < (int)logEntries.size() - maxVisibleLines) {
            lcd->setTextColor(0xFFFF);
            lcd->setTextSize(1);
            lcd->setCursor(indicatorX, lcd->height() - FOOTER_HEIGHT - UIScale::scale(10));
            lcd->print("v");
        }
    }

    // Draw log counter in corner
    lcd->setTextColor(0x8410);  // Gray
    lcd->setTextSize(1);
    lcd->setCursor(UIScale::scale(2), lcd->height() - FOOTER_HEIGHT - UIScale::scale(12));
    lcd->print(String(logEntries.size()) + "/" + String(MAX_LOG_ENTRIES));
}

void LogViewerScreen::handleScrolling(int x, int y, bool wasTapped) {
    if (!wasTapped || logEntries.size() <= maxVisibleLines) return;

    // Check for scroll in log area
    if (y >= HEADER_HEIGHT && y < lcd->height() - FOOTER_HEIGHT) {
        int logAreaHeight = lcd->height() - HEADER_HEIGHT - FOOTER_HEIGHT;

        // Top half scrolls up, bottom half scrolls down
        if (y < HEADER_HEIGHT + logAreaHeight / 2) {
            scrollUp();
        } else {
            scrollDown();
        }
    }
}

void LogViewerScreen::scrollUp() {
    if (scrollOffset > 0) {
        scrollOffset--;
        markForRedraw();
    }
}

void LogViewerScreen::scrollDown() {
    int maxScroll = std::max(0, (int)logEntries.size() - maxVisibleLines);
    if (scrollOffset < maxScroll) {
        scrollOffset++;
        markForRedraw();
    }
}

uint16_t LogViewerScreen::getLevelColor(int level) {
    switch (level) {
        case 0:
            return 0x8410;  // VERBOSE - Gray
        case 1:
            return 0xFFFF;  // DEBUG - White
        case 2:
            return 0xFFFF;  // INFO - White
        case 3:
            return 0xFFE0;  // WARN - Yellow
        case 4:
            return 0xF800;  // ERROR - Red
        default:
            return 0xFFFF;  // White
    }
}

String LogViewerScreen::getLevelString(int level) {
    switch (level) {
        case 0:
            return "V";
        case 1:
            return "D";
        case 2:
            return "I";
        case 3:
            return "W";
        case 4:
            return "E";
        default:
            return "?";
    }
}

}  // namespace Screens
}  // namespace UI
}  // namespace BTLogger