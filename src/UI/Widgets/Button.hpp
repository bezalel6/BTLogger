#pragma once

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <Arduino.h>
#include <functional>

namespace BTLogger {
namespace UI {
namespace Widgets {

// RGB565 color definitions for LovyanGFX
namespace Colors {
const uint16_t WHITE = 0xFFFF;
const uint16_t BLACK = 0x0000;
const uint16_t RED = 0xF800;
const uint16_t GREEN = 0x07E0;
const uint16_t BLUE = 0x001F;
const uint16_t YELLOW = 0xFFE0;
const uint16_t CYAN = 0x07FF;
const uint16_t MAGENTA = 0xF81F;
const uint16_t GRAY = 0x8410;
const uint16_t LIGHT_GRAY = 0xC618;
const uint16_t DARK_GRAY = 0x4208;
const uint16_t ORANGE = 0xFD20;
const uint16_t PURPLE = 0x780F;
const uint16_t BROWN = 0xA145;
const uint16_t PINK = 0xF81F;

// Button specific colors
const uint16_t BLUE_LIGHT = 0x051F;
const uint16_t GREEN_LIGHT = 0x07E8;
const uint16_t RED_LIGHT = 0xF810;
}  // namespace Colors

/**
 * Button widget with automatic text width calculation and proper sizing
 */
class Button {
   public:
    Button(lgfx::LGFX_Device& display, int x, int y, int w, int h, const String& label);
    ~Button();

    // Core functionality
    void draw();
    void update();
    void handleTouch(int x, int y, bool touched);

    // Properties
    void setText(const String& newText);
    void setEnabled(bool enable);
    void setCallback(std::function<void()> cb);
    void setColors(uint16_t bg, uint16_t bgPress, uint16_t border, uint16_t txt);
    void setPosition(int x, int y);

    // State
    bool isPressed() const;
    bool isEnabled() const;

    // Getters for layout purposes
    int getX() const { return posX; }
    int getY() const { return posY; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }

   private:
    lgfx::LGFX_Device* lcd;

    // Position and size
    int posX, posY;
    int width, height;

    // Appearance
    String text;
    uint16_t bgColor, bgColorPressed;
    uint16_t borderColor, borderColorPressed;
    uint16_t textColor, textColorPressed;

    // State
    bool pressed;
    bool enabled;

    // Callback
    std::function<void()> callback;

    // Helper methods
    void adjustWidthForText();
};

}  // namespace Widgets
}  // namespace UI
}  // namespace BTLogger