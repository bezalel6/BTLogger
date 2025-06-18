#include "ScrollingText.hpp"
#include "../UIScale.hpp"

namespace BTLogger {
namespace UI {
namespace Widgets {

ScrollingText::ScrollingText(lgfx::LGFX_Device& display, int x, int y, int maxWidth, int textSize)
    : lcd(&display), posX(x), posY(y), maxWidth(maxWidth), textSize(textSize), textColor(0xFFFF), backgroundColor(0x0000), textWidth(0), textNeedsScrolling(false), scrollingEnabled(true), paused(false), scrollOffset(0), scrollDirection(true), lastUpdateTime(0), lastDirectionChange(0), scrollSpeed(30), scrollDelay(1000), pauseAtEnds(1500) {
    calculateTextMetrics();
}

ScrollingText::~ScrollingText() {
}

void ScrollingText::setText(const String& newText) {
    text = newText;
    scrollOffset = 0;
    scrollDirection = true;
    lastUpdateTime = millis();
    lastDirectionChange = millis();
    calculateTextMetrics();
}

void ScrollingText::setColors(uint16_t textCol, uint16_t bgCol) {
    textColor = textCol;
    backgroundColor = bgCol;
}

void ScrollingText::setPosition(int x, int y) {
    posX = x;
    posY = y;
}

void ScrollingText::setMaxWidth(int width) {
    maxWidth = width;
    calculateTextMetrics();
}

void ScrollingText::setTextSize(int size) {
    textSize = size;
    calculateTextMetrics();
}

void ScrollingText::startScrolling() {
    scrollingEnabled = true;
    paused = false;
    lastUpdateTime = millis();
}

void ScrollingText::stopScrolling() {
    scrollingEnabled = false;
    scrollOffset = 0;
    scrollDirection = true;
}

void ScrollingText::pauseScrolling() {
    paused = true;
}

void ScrollingText::resumeScrolling() {
    paused = false;
    lastUpdateTime = millis();
}

void ScrollingText::resetScrolling() {
    scrollOffset = 0;
    scrollDirection = true;
    lastUpdateTime = millis();
    lastDirectionChange = millis();
}

void ScrollingText::update() {
    if (!scrollingEnabled || !textNeedsScrolling || paused) {
        return;
    }

    unsigned long currentTime = millis();
    unsigned long deltaTime = currentTime - lastUpdateTime;

    // Apply scroll delay before starting
    if (currentTime - lastDirectionChange < scrollDelay) {
        return;
    }

    // Calculate new scroll position based on speed
    int pixelsToMove = (deltaTime * scrollSpeed) / 1000;
    if (pixelsToMove > 0) {
        updateScrollPosition();
        lastUpdateTime = currentTime;
    }
}

void ScrollingText::draw() {
    if (!lcd || text.isEmpty()) return;

    // Clear the background
    lcd->fillRect(posX, posY, maxWidth, UIScale::calculateTextHeight(textSize), backgroundColor);

    // Set text properties
    lcd->setTextSize(textSize);
    lcd->setTextColor(textColor);

    if (!textNeedsScrolling) {
        // Text fits, draw normally centered
        int textHeight = UIScale::calculateTextHeight(textSize);
        int yOffset = (UIScale::calculateTextHeight(textSize) - textHeight) / 2;
        lcd->setCursor(posX, posY + yOffset);
        lcd->print(text);
    } else {
        // Text needs scrolling, draw with clipping
        drawClippedText();
    }
}

void ScrollingText::calculateTextMetrics() {
    if (text.isEmpty()) {
        textWidth = 0;
        textNeedsScrolling = false;
        return;
    }

    textWidth = UIScale::calculateTextWidth(text, textSize);
    textNeedsScrolling = (textWidth > maxWidth);

    if (!textNeedsScrolling) {
        scrollOffset = 0;
    }
}

void ScrollingText::updateScrollPosition() {
    if (!textNeedsScrolling) return;

    int maxOffset = getMaxScrollOffset();

    if (scrollDirection) {
        // Scrolling right to left (text moving left)
        scrollOffset++;
        if (scrollOffset >= maxOffset) {
            scrollOffset = maxOffset;
            scrollDirection = false;
            lastDirectionChange = millis();
        }
    } else {
        // Scrolling left to right (text moving right)
        scrollOffset--;
        if (scrollOffset <= 0) {
            scrollOffset = 0;
            scrollDirection = true;
            lastDirectionChange = millis();
        }
    }
}

void ScrollingText::drawClippedText() {
    if (!lcd) return;

    // Set up clipping rectangle
    int clipX = posX;
    int clipY = posY;
    int clipW = maxWidth;
    int clipH = UIScale::calculateTextHeight(textSize);

    // Save current clip region
    lcd->setClipRect(clipX, clipY, clipW, clipH);

    // Calculate text position with scroll offset
    int textX = posX - scrollOffset;
    int textY = posY;

    // Draw the text
    lcd->setCursor(textX, textY);
    lcd->print(text);

    // Restore clipping (remove clip)
    lcd->clearClipRect();
}

int ScrollingText::getMaxScrollOffset() const {
    return std::max(0, textWidth - maxWidth + UIScale::scale(5));  // Small buffer
}

}  // namespace Widgets
}  // namespace UI
}  // namespace BTLogger