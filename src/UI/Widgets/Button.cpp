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
        // Touch started - activate immediately
        pressed = true;
        pressTime = millis();
        lastTouchState = true;
        Serial.printf("Button '%s' activated\n", text.c_str());

        // Execute callback immediately on press
        if (callback) {
            callback();
        }
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
    // Draw shadow (offset by 2 pixels)
    if (enabled && !pressed) {
        lcd.fillRoundRect(x + 2, y + 2, width, height, UIScale::scale(5), Colors::BLACK);
    }

    // Draw background
    lcd.fillRoundRect(x, y, width, height, UIScale::scale(5), backgroundColor);

    // Draw 3D border effect
    if (enabled) {
        if (pressed) {
            // Pressed state - darker border
            lcd.drawRoundRect(x, y, width, height, UIScale::scale(5), Colors::GRAY);
            lcd.drawRoundRect(x + 1, y + 1, width - 2, height - 2, UIScale::scale(4), Colors::WHITE);
        } else {
            // Normal state - raised border effect
            lcd.drawRoundRect(x, y, width, height, UIScale::scale(5), Colors::WHITE);
            lcd.drawRoundRect(x + 1, y + 1, width - 2, height - 2, UIScale::scale(4), Colors::LIGHT_GRAY);
        }
    } else {
        // Disabled state - simple gray border
        lcd.drawRoundRect(x, y, width, height, UIScale::scale(5), Colors::GRAY);
    }

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