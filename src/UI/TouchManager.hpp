#pragma once

#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <Preferences.h>

namespace BTLogger {
namespace UI {

/**
 * TouchManager handles touch input and calibration for XPT2046 touch controller
 * Uses LovyanGFX's built-in calibration system for reliability
 */
class TouchManager {
   public:
    // Touch point structure
    struct TouchPoint {
        int x, y;
        bool pressed;

        TouchPoint() : x(0), y(0), pressed(false) {}
        TouchPoint(int _x, int _y, bool _pressed = true) : x(_x), y(_y), pressed(_pressed) {}
    };

    // Initialization
    static bool initialize(lgfx::LGFX_Device& display);
    static bool isInitialized() { return initialized; }

    // Main update loop
    static void update();

    // Touch state
    static TouchPoint getTouch() { return currentTouch; }
    static bool isTouched() { return currentTouch.pressed; }
    static bool wasTapped() { return tapped; }
    static void clearTap() { tapped = false; }

    // Touch area checking
    static bool isTouchedInArea(int x, int y, int width, int height);
    static bool wasTappedInArea(int x, int y, int width, int height);

    // Debug and testing
    static void showTouchDebugInfo();

    // Calibration - uses LovyanGFX built-in system
    static bool needsCalibration();
    static void startCalibration();
    static bool isCalibrating() { return calibrating; }
    static void resetCalibration();

    // Configuration
    static void setDebounceTime(unsigned long ms) { debounceTime = ms; }

   private:
    static bool initialized;
    static lgfx::LGFX_Device* lcd;
    static Preferences preferences;

    // Touch state
    static TouchPoint currentTouch;
    static TouchPoint lastTouch;
    static bool tapped;
    static unsigned long lastTouchTime;
    static unsigned long debounceTime;

    // Calibration
    static bool calibrating;

    // Calibration methods using LovyanGFX built-in system
    static bool hasSavedCalibration();
    static bool saveTouchCalibration(std::uint16_t* calData);
    static bool loadTouchCalibration(std::uint16_t* calData);
    static void performTouchCalibration();
    static void clearTouchCalibration();

    // Touch coordinate retrieval
    static TouchPoint getTouchCoordinates();
};

}  // namespace UI
}  // namespace BTLogger