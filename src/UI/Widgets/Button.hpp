#pragma once

#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <functional>

namespace BTLogger {
namespace UI {
namespace Widgets {

// RGB565 color definitions for LovyanGFX
namespace Colors {
const uint16_t WHITE = 0xFFFF;
const uint16_t BLACK = 0x0000;
const uint16_t BLUE = 0x001F;
const uint16_t BLUE_DARK = 0x0014;
const uint16_t BLUE_LIGHT = 0x3D1F;
const uint16_t GRAY = 0x8410;
const uint16_t GRAY_LIGHT = 0xC618;
const uint16_t GRAY_DARK = 0x4208;
}  // namespace Colors

/**
 * Button widget with touch interaction and visual feedback
 * Designed for simple tap interactions with press/release states
 */
class Button {
   public:
    // Button callback type
    using ButtonCallback = std::function<void()>;

    // Constructor
    Button(lgfx::LGFX_Device& display, int x, int y, int width, int height, const String& text);

    // Configuration
    void setText(const String& text) { this->text = text; }
    void setCallback(ButtonCallback callback) { this->callback = callback; }
    void setEnabled(bool enabled) { this->enabled = enabled; }
    void setColors(uint16_t normal, uint16_t pressed, uint16_t disabled, uint16_t textColor);

    // State
    bool isEnabled() const { return enabled; }
    bool isPressed() const { return pressed; }
    bool isVisible() const { return visible; }
    void setVisible(bool visible) { this->visible = visible; }

    // Geometry
    void setPosition(int x, int y) {
        this->x = x;
        this->y = y;
    }
    void setSize(int width, int height) {
        this->width = width;
        this->height = height;
    }
    void getBounds(int& outX, int& outY, int& outWidth, int& outHeight) const;

    // Interaction
    void update();  // Call this in main loop
    bool handleTouch(int touchX, int touchY, bool touched);

    // Rendering
    void draw();
    void drawPressed();
    void drawNormal();

   private:
    lgfx::LGFX_Device& lcd;

    // Geometry
    int x, y;
    int width, height;

    // Appearance
    String text;
    uint16_t colorNormal;
    uint16_t colorPressed;
    uint16_t colorDisabled;
    uint16_t colorText;

    // State
    bool enabled;
    bool visible;
    bool pressed;
    bool lastTouchState;
    unsigned long pressTime;

    // Interaction
    ButtonCallback callback;

    // Constants
    static const unsigned long PRESS_DURATION = 100;  // Visual feedback duration
    static const int FONT_SIZE = 2;

    // Internal methods
    bool isPointInside(int px, int py) const;
    void drawButton(uint16_t backgroundColor);
    int getTextX() const;
    int getTextY() const;
};

}  // namespace Widgets
}  // namespace UI
}  // namespace BTLogger