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
    if (paused) return;

    logEntries.emplace_back(deviceName, tag, message, level);

    // Limit number of entries to prevent memory issues
    if (logEntries.size() > MAX_LOG_ENTRIES) {
        logEntries.erase(logEntries.begin());
    }

    // Auto-scroll to bottom
    int maxScroll = std::max(0, (int)logEntries.size() - maxVisibleLines);
    scrollOffset = maxScroll;

    markForRedraw();
}

void LogViewerScreen::clearLogs() {
    logEntries.clear();
    scrollOffset = 0;
    markForRedraw();
    ScreenManager::setStatusText("Logs cleared");
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

    // Clear log area
    int logAreaY = HEADER_HEIGHT;
    int logAreaHeight = lcd->height() - HEADER_HEIGHT - FOOTER_HEIGHT;
    lcd->fillRect(0, logAreaY, lcd->width(), logAreaHeight, 0x0000);

    if (logEntries.empty()) {
        lcd->setTextColor(0x8410);  // Gray
        lcd->setTextSize(UIScale::scale(1));
        lcd->setCursor(UIScale::scale(10), logAreaY + UIScale::scale(20));
        lcd->print("No logs available");
        return;
    }

    // Draw visible log entries
    int y = logAreaY + UIScale::scale(5);
    int lineHeight = UIScale::scale(LINE_HEIGHT);
    int startIndex = scrollOffset;
    int endIndex = std::min(startIndex + maxVisibleLines, (int)logEntries.size());

    for (int i = startIndex; i < endIndex; i++) {
        const LogEntry& entry = logEntries[i];

        // Set color based on log level
        lcd->setTextColor(getLevelColor(entry.level));
        lcd->setTextSize(UIScale::scale(1));

        // Format: [LEVEL] Device: Message
        String line = "[" + getLevelString(entry.level) + "] " +
                      entry.deviceName + ": " + entry.message;

        // Truncate if too long
        if (line.length() > 35) {
            line = line.substring(0, 32) + "...";
        }

        lcd->setCursor(UIScale::scale(5), y);
        lcd->print(line);

        y += lineHeight;
        if (y >= lcd->height() - FOOTER_HEIGHT) break;
    }

    // Draw scroll indicators
    if (logEntries.size() > maxVisibleLines) {
        int indicatorX = lcd->width() - UIScale::scale(10);

        if (scrollOffset > 0) {
            lcd->setTextColor(0xFFFF);
            lcd->setCursor(indicatorX, logAreaY + UIScale::scale(5));
            lcd->print("^");
        }

        if (scrollOffset < (int)logEntries.size() - maxVisibleLines) {
            lcd->setTextColor(0xFFFF);
            lcd->setCursor(indicatorX, lcd->height() - FOOTER_HEIGHT - UIScale::scale(15));
            lcd->print("v");
        }
    }
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