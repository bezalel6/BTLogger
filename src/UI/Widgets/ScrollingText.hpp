#pragma once

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <Arduino.h>

namespace BTLogger {
namespace UI {
namespace Widgets {

/**
 * ScrollingText widget for animating long text that doesn't fit in available space
 * Provides smooth horizontal scrolling animation for better readability
 */
class ScrollingText {
   public:
    ScrollingText(lgfx::LGFX_Device& display, int x, int y, int maxWidth, int textSize);
    ~ScrollingText();

    // Core functionality
    void setText(const String& text);
    void setColors(uint16_t textColor, uint16_t bgColor);
    void setPosition(int x, int y);
    void setMaxWidth(int width);
    void setTextSize(int size);

    // Animation control
    void startScrolling();
    void stopScrolling();
    void pauseScrolling();
    void resumeScrolling();
    void resetScrolling();

    // Update and render
    void update();
    void draw();

    // State queries
    bool isScrolling() const { return scrollingEnabled && textNeedsScrolling; }
    bool isAnimating() const { return scrollingEnabled && textNeedsScrolling && !paused; }

    // Configuration
    void setScrollSpeed(int pixelsPerSecond) { scrollSpeed = pixelsPerSecond; }
    void setScrollDelay(unsigned long delayMs) { scrollDelay = delayMs; }
    void setPauseAtEnds(unsigned long pauseMs) { pauseAtEnds = pauseMs; }

   private:
    lgfx::LGFX_Device* lcd;

    // Position and dimensions
    int posX, posY;
    int maxWidth;
    int textSize;

    // Colors
    uint16_t textColor;
    uint16_t backgroundColor;

    // Text content
    String text;
    int textWidth;
    bool textNeedsScrolling;

    // Animation state
    bool scrollingEnabled;
    bool paused;
    int scrollOffset;
    bool scrollDirection;  // true = right to left, false = left to right
    unsigned long lastUpdateTime;
    unsigned long lastDirectionChange;

    // Animation settings
    int scrollSpeed;            // pixels per second
    unsigned long scrollDelay;  // delay before starting scroll (ms)
    unsigned long pauseAtEnds;  // pause time at start/end positions (ms)

    // Helper methods
    void calculateTextMetrics();
    void updateScrollPosition();
    void drawClippedText();
    int getMaxScrollOffset() const;
};

}  // namespace Widgets
}  // namespace UI
}  // namespace BTLogger