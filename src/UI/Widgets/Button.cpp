#include "Button.hpp"
#include "../UIScale.hpp"

namespace BTLogger {
namespace UI {
namespace Widgets {

Button::Button(lgfx::LGFX_Device& display, int x, int y, int width, int height, const String& text)
    : lcd(display), x(x), y(y), width(width), height(height), text(text), colorNormal(Colors::BLUE), colorPressed(Colors::BLUE_LIGHT), colorDisabled(Colors::GRAY), colorText(Colors::WHITE), enabled(true), visible(true), pressed(false), lastTouchState(false), pressTime(0) {
}

void Button::setColors(uint16_t normal, uint16_t pressed, uint16_t disabled, uint16_t textColor) {
    colorNormal = normal;
    colorPressed = pressed;
    colorDisabled = disabled;
    colorText = textColor;
}

void Button::getBounds(int& outX, int& outY, int& outWidth, int& outHeight) const {
    outX = x;
    outY = y;
    outWidth = width;
    outHeight = height;
}

void Button::update() {
    if (!visible || !enabled) {
        return;
    }

    // Handle press animation timeout
    if (pressed && (millis() - pressTime > PRESS_DURATION)) {
        pressed = false;
    }
}

bool Button::handleTouch(int touchX, int touchY, bool touched) {
    if (!visible || !enabled) {
        return false;
    }

    bool wasInside = isPointInside(touchX, touchY);

    if (touched && wasInside && !lastTouchState) {
        // Touch started
        pressed = true;
        pressTime = millis();
        lastTouchState = true;
        return true;
    } else if (!touched && lastTouchState && wasInside) {
        // Touch ended inside button
        if (callback) {
            callback();
        }
        lastTouchState = false;
        return true;
    } else if (!touched) {
        lastTouchState = false;
    }

    return false;
}

void Button::draw() {
    if (!visible) {
        return;
    }

    if (!enabled) {
        drawButton(colorDisabled);
    } else if (pressed) {
        drawButton(colorPressed);
    } else {
        drawButton(colorNormal);
    }
}

void Button::drawPressed() {
    if (visible) {
        drawButton(colorPressed);
    }
}

void Button::drawNormal() {
    if (visible) {
        drawButton(enabled ? colorNormal : colorDisabled);
    }
}

bool Button::isPointInside(int px, int py) const {
    return (px >= x && px < x + width && py >= y && py < y + height);
}

void Button::drawButton(uint16_t backgroundColor) {
    // Draw background
    lcd.fillRoundRect(x, y, width, height, UIScale::scale(5), backgroundColor);

    // Draw border
    lcd.drawRoundRect(x, y, width, height, UIScale::scale(5), Colors::WHITE);

    // Draw text
    lcd.setTextColor(colorText);
    lcd.setTextSize(FONT_SIZE);

    int textX = getTextX();
    int textY = getTextY();

    lcd.setCursor(textX, textY);
    lcd.print(text);
}

int Button::getTextX() const {
    // Center text horizontally (approximate)
    int textWidth = text.length() * FONT_SIZE * 6;  // Rough calculation
    return x + (width - textWidth) / 2;
}

int Button::getTextY() const {
    // Center text vertically (approximate)
    int textHeight = FONT_SIZE * 8;  // Rough calculation
    return y + (height - textHeight) / 2;
}

}  // namespace Widgets
}  // namespace UI
}  // namespace BTLogger