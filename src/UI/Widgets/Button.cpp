#include "Button.hpp"
#include "../UIScale.hpp"

namespace BTLogger {
namespace UI {
namespace Widgets {

Button::Button(lgfx::LGFX_Device& display, int x, int y, int w, int h, const String& label)
    : lcd(&display), posX(x), posY(y), width(w), height(h), text(label), bgColor(0x0000), borderColor(0x8410), textColor(0xFFFF), bgColorPressed(0x8410), borderColorPressed(0xFFFF), textColorPressed(0x0000), pressed(false), enabled(true), callback(nullptr), scrollingText(nullptr), useScrollingText(false) {
    // Auto-adjust width if it's too small for the text
    adjustWidthForText();
    // Initialize scrolling text if needed
    initializeScrollingText();
}

Button::~Button() {
    delete scrollingText;
}

void Button::adjustWidthForText() {
    int textSize = UIScale::getButtonTextSize();
    int requiredWidth = UIScale::calculateTextWidth(text, textSize) + UIScale::scale(16);  // 8px padding on each side

    if (width < requiredWidth) {
        width = requiredWidth;
        Serial.printf("Button '%s' width adjusted from %d to %d (text size %d)\n",
                      text.c_str(), width - (requiredWidth - width), width, textSize);
    }
}

void Button::initializeScrollingText() {
    if (!lcd || text.isEmpty()) return;

    int textSize = UIScale::getButtonTextSize();
    int textWidth = UIScale::calculateTextWidth(text, textSize);
    int availableWidth = width - UIScale::scale(16);  // Account for padding

    // Only use scrolling text if the text is too long
    if (textWidth > availableWidth && availableWidth > 0) {
        if (!scrollingText) {
            scrollingText = new ScrollingText(*lcd, 0, 0, availableWidth, textSize);
        }
        scrollingText->setText(text);
        scrollingText->setMaxWidth(availableWidth);
        scrollingText->setTextSize(textSize);
        scrollingText->startScrolling();
        useScrollingText = true;
    } else {
        // Text fits, no need for scrolling
        useScrollingText = false;
        if (scrollingText) {
            scrollingText->stopScrolling();
        }
    }
}

void Button::updateScrollingTextProperties() {
    if (!scrollingText) return;

    // Calculate text area within button
    int textSize = UIScale::getButtonTextSize();
    int textHeight = UIScale::calculateTextHeight(textSize);
    int availableWidth = width - UIScale::scale(16);  // Account for padding

    int textX = posX + UIScale::scale(8);          // Left padding
    int textY = posY + (height - textHeight) / 2;  // Vertically centered

    scrollingText->setPosition(textX, textY);
    scrollingText->setMaxWidth(availableWidth);
    scrollingText->setTextSize(textSize);
}

void Button::setText(const String& newText) {
    text = newText;
    adjustWidthForText();
    initializeScrollingText();
}

void Button::draw() {
    if (!lcd) return;

    // Choose colors based on state
    uint16_t currentBg = pressed ? bgColorPressed : bgColor;
    uint16_t currentBorder = pressed ? borderColorPressed : borderColor;
    uint16_t currentText = pressed ? textColorPressed : textColor;

    // Draw button background
    lcd->fillRect(posX, posY, width, height, currentBg);

    // Draw border
    lcd->drawRect(posX, posY, width, height, currentBorder);

    // Draw text
    if (!text.isEmpty()) {
        if (useScrollingText && scrollingText) {
            // Update scrolling text properties and draw
            updateScrollingTextProperties();
            scrollingText->setColors(currentText, currentBg);
            scrollingText->draw();
        } else {
            // Static text drawing (fallback)
            int textSize = UIScale::getButtonTextSize();
            lcd->setTextSize(textSize);
            lcd->setTextColor(currentText);

            // Calculate text position for centering
            int textWidth = UIScale::calculateTextWidth(text, textSize);
            int textHeight = UIScale::calculateTextHeight(textSize);

            int textX = posX + (width - textWidth) / 2;
            int textY = posY + (height - textHeight) / 2;

            // Ensure text doesn't go outside button bounds
            textX = constrain(textX, posX + 2, posX + width - textWidth - 2);
            textY = constrain(textY, posY + 2, posY + height - textHeight - 2);

            lcd->setCursor(textX, textY);
            lcd->print(text);
        }
    }
}

void Button::update() {
    // Button updates are handled via touch events

    // Update scrolling text animation
    if (useScrollingText && scrollingText) {
        scrollingText->update();
    }
}

bool Button::handleTouch(int x, int y, bool touched) {
    if (!enabled || !lcd) return false;

    bool wasPressed = pressed;
    bool touchInside = (x >= posX && x < posX + width && y >= posY && y < posY + height);

    if (touched && touchInside) {
        if (!pressed) {
            pressed = true;
            Serial.printf("Button '%s' pressed\n", text.c_str());

            // Trigger callback immediately on press
            if (callback) {
                Serial.printf("Button '%s' activated\n", text.c_str());
                callback();
            }
        }
    } else {
        if (pressed) {
            pressed = false;
        }
    }

    // Redraw if state changed
    if (pressed != wasPressed) {
        draw();
    }

    // Return true if touch was inside this button's area
    return touchInside;
}

bool Button::isPressed() const {
    return pressed;
}

bool Button::isEnabled() const {
    return enabled;
}

void Button::setEnabled(bool enable) {
    if (enabled != enable) {
        enabled = enable;
        pressed = false;  // Clear pressed state when disabled
        draw();
    }
}

void Button::setCallback(std::function<void()> cb) {
    callback = cb;
}

void Button::setColors(uint16_t bg, uint16_t bgPress, uint16_t border, uint16_t txt) {
    bgColor = bg;
    bgColorPressed = bgPress;
    borderColor = border;
    textColor = txt;

    // Set pressed text color to contrast with pressed background
    textColorPressed = (bgPress == 0x0000) ? 0xFFFF : 0x0000;
    borderColorPressed = (border == 0x8410) ? 0xFFFF : border;
}

void Button::setPosition(int x, int y) {
    posX = x;
    posY = y;
}

void Button::pauseScrolling() {
    if (scrollingText) {
        scrollingText->pauseScrolling();
    }
}

void Button::resumeScrolling() {
    if (scrollingText) {
        scrollingText->resumeScrolling();
    }
}

}  // namespace Widgets
}  // namespace UI
}  // namespace BTLogger