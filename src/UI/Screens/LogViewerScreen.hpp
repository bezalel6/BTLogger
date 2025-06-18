#pragma once

#include "../Screen.hpp"
#include "../Widgets/Button.hpp"
#include "../../Core/BluetoothManager.hpp"
#include <vector>

namespace BTLogger {
namespace UI {
namespace Screens {

/**
 * Log viewer screen for displaying real-time logs from connected devices
 */
class LogViewerScreen : public Screen {
   public:
    LogViewerScreen();
    virtual ~LogViewerScreen();

    // Screen interface
    void activate() override;
    void deactivate() override;
    void update() override;
    void handleTouch(int x, int y, bool touched) override;
    void cleanup() override;

    // Log management
    void addLogEntry(const String& deviceName, const String& tag, const String& message, int level);
    void clearLogs();

   private:
    struct LogEntry {
        String deviceName;
        String tag;
        String message;
        int level;
        unsigned long timestamp;

        LogEntry(const String& dev, const String& t, const String& msg, int lvl)
            : deviceName(dev), tag(t), message(msg), level(lvl), timestamp(millis()) {}
    };

    // UI Elements
    Widgets::Button* backButton;
    Widgets::Button* clearButton;
    Widgets::Button* pauseButton;

    // Log data
    std::vector<LogEntry> logEntries;
    int scrollOffset;
    int maxVisibleLines;
    bool paused;
    bool lastTouchState;

    // Constants
    static const int MAX_LOG_ENTRIES = 100;
    static const int LINE_HEIGHT = 12;

    void drawHeader();
    void drawLogs();
    void scrollUp();
    void scrollDown();
    void handleScrolling(int x, int y, bool wasTapped);
    uint16_t getLevelColor(int level);
    String getLevelString(int level);
};

}  // namespace Screens
}  // namespace UI
}  // namespace BTLogger