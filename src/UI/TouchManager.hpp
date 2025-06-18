#pragma once

#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <Preferences.h>

namespace BTLogger {
namespace UI {

/**
 * TouchManager handles touch input and calibration for XPT2046 touch controller
 * Provides simple tap-based interaction with coordinate transformation
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

    // Calibration data
    struct CalibrationData {
        int x_min, x_max, y_min, y_max;
        bool valid;

        CalibrationData() : x_min(0), x_max(240), y_min(0), y_max(320), valid(false) {}
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

    // Calibration
    static bool needsCalibration();
    static void startCalibration();
    static bool isCalibrating() { return calibrating; }
    static void resetCalibration();

    // Configuration
    static void setDebounceTime(unsigned long ms) { debounceTime = ms; }
    static void setTouchThreshold(int threshold) { touchThreshold = threshold; }

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
    static int touchThreshold;

    // Calibration
    static bool calibrating;
    static int calibrationStep;
    static CalibrationData calibrationData;
    static TouchPoint calibrationPoints[4];  // Four corners
    static TouchPoint rawCalibrationPoints[4];

    // Internal methods
    static TouchPoint readRawTouch();
    static TouchPoint transformTouch(const TouchPoint& raw);
    static void saveCalibration();
    static void loadCalibration();
    static void drawCalibrationScreen();
    static void drawCalibrationPoint(int x, int y, bool active = false);
    static void processCalibrationTouch(const TouchPoint& raw);
    static void calculateCalibration();
};

}  // namespace UI
}  // namespace BTLogger