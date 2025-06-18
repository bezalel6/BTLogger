/**
 * TouchManager - Enhanced touch input manager with advanced calibration
 *
 * This class provides touch input management with both raw and calibrated coordinates.
 * It supports both software SPI (bitbang) and hardware SPI touch controllers.
 *
 * BASIC USAGE:
 * 1. Initialize: TouchManager::initialize(display)
 * 2. Update in loop: TouchManager::update()
 * 3. Get touch: TouchManager::getTouch()
 *
 * CALIBRATION USAGE:
 * Option 1 - Automatic calibration (recommended for first-time setup):
 *   TouchManager::startImprovedCalibration();
 *
 * Option 2 - Manual calibration (if you know the values):
 *   TouchManager::setCalibration(xMin, xMax, yMin, yMax);
 *   TouchManager::setRotation(rotation); // 0-3, same as display rotation
 *
 * After calibration, TouchManager::getTouch() returns TouchPoint with:
 * - .x, .y = raw touch coordinates
 * - .calx, .caly = calibrated screen coordinates (matches display pixels)
 * - .pressed = true if currently touched
 *
 * The calibrated coordinates automatically handle rotation and screen mapping.
 */

#pragma once

#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <Preferences.h>

// Alternative software SPI touch implementation for CYD SPI conflict resolution
#ifdef USE_BITBANG_TOUCH
#include <XPT2046_Bitbang.h>
#endif

namespace BTLogger {
namespace UI {

/**
 * TouchManager handles touch input and calibration for XPT2046 touch controller
 * Uses LovyanGFX's built-in calibration system for reliability
 *
 * Calibration versioning: The system uses a compile-time version number
 * (TOUCH_CALIBRATION_VERSION in TouchManager.cpp) to invalidate existing
 * calibration data when needed. Increment this value to force re-calibration
 * on all devices while allowing new calibration data to be saved.
 */
class TouchManager {
   public:
    // Touch point structure
    struct TouchPoint {
        int x, y;
        bool pressed;

        // Calibrated coordinates (for use after calibration)
        int calx, caly;

        TouchPoint() : x(0), y(0), pressed(false), calx(0), caly(0) {}
    };

    // Calibration data structure
    struct CalibrationData {
        uint16_t xMin, xMax, yMin, yMax;
        int screenWidth, screenHeight;
        uint8_t rotation;
        bool isValid;

        CalibrationData() : xMin(0), xMax(4095), yMin(0), yMax(4095), screenWidth(320), screenHeight(240), rotation(0), isValid(false) {}
    };

    // Initialization
    static bool initialize(lgfx::LGFX_Device& display);
    static bool isInitialized() { return initialized; }

    // Main update loop
    static void update();

    // Touch state
    static TouchPoint getTouch();  // Returns calibrated coordinates if available, raw otherwise
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
    static void startCalibration();          // Uses old calibration routine
    static void startImprovedCalibration();  // Uses new improved routine
    static bool isCalibrating() { return calibrating; }
    static void resetCalibration();

    // Configuration
    static void setDebounceTime(unsigned long ms) { debounceTime = ms; }

    // Enhanced calibration methods
    static void setCalibration(uint16_t xMin, uint16_t xMax, uint16_t yMin, uint16_t yMax);
    static void setRotation(uint8_t rotation);
    static TouchPoint getCalibratedPoint();
    static bool isCalibrated() { return calibrationData.isValid; }

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

    // Touch coordinate retrieval (internal use)
    static TouchPoint getTouchCoordinates();

#ifdef USE_BITBANG_TOUCH
    // Software SPI touch implementation for CYD SPI conflict resolution
    static XPT2046_Bitbang* touchController;
    static TouchPoint getBitbangTouchCoordinates();
    static bool initializeBitbangTouch();

    // Bitbang touch calibration methods
    static void performBitbangTouchCalibration();
    static void drawCalibrationCrosshair(int x, int y);

    // Touch pin definitions are used from Hardware/ESP32_SPI_9341.h macros:
    // TOUCH_IRQ, TOUCH_MOSI, TOUCH_MISO, TOUCH_SCK, TOUCH_CS
#endif

    static CalibrationData calibrationData;

    // Coordinate transformation methods
    static void transformRawToScreen(int rawX, int rawY, int& screenX, int& screenY);
    static void applyRotation(int& x, int& y);
    static bool improvedCalibrationRoutine();
};

}  // namespace UI
}  // namespace BTLogger