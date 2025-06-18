#include "TouchManager.hpp"
#include "UIScale.hpp"
#include <algorithm>

#ifdef USE_BITBANG_TOUCH
#include "../Hardware/ESP32_SPI_9341.h"
#endif

namespace BTLogger {
namespace UI {

// Static member definitions
bool TouchManager::initialized = false;
lgfx::LGFX_Device* TouchManager::lcd = nullptr;
Preferences TouchManager::preferences;

#ifdef USE_BITBANG_TOUCH
XPT2046_Bitbang* TouchManager::touchController = nullptr;
#endif

TouchManager::TouchPoint TouchManager::currentTouch;
TouchManager::TouchPoint TouchManager::lastTouch;
bool TouchManager::tapped = false;
unsigned long TouchManager::lastTouchTime = 0;
unsigned long TouchManager::debounceTime = 50;

bool TouchManager::calibrating = false;

bool TouchManager::initialize(lgfx::LGFX_Device& display) {
    if (initialized) {
        return true;
    }

    lcd = &display;

#ifdef USE_BITBANG_TOUCH
    // Use software SPI touch to avoid SPI conflict with SD card
    if (!initializeBitbangTouch()) {
        Serial.println("Failed to initialize software SPI touch");
        return false;
    }
    Serial.println("TouchManager initialized with software SPI (bitbang) touch");
#else
    // Use LovyanGFX hardware SPI touch
    // Check if we have saved calibration data
    if (hasSavedCalibration()) {
        // Load and apply saved calibration
        std::uint16_t calData[8];  // Standard calibration data array size
        if (loadTouchCalibration(calData)) {
            lcd->setTouchCalibrate(calData);
            Serial.println("Touch calibration loaded from storage");
        } else {
            Serial.println("Failed to load touch calibration, performing new calibration");
            performTouchCalibration();
        }
    } else {
        // No saved calibration found, use default calibration values
        Serial.println("No saved touch calibration found, using default values");
        std::uint16_t defaultCalData[8] = {239, 0, 319, 0, 0, 239, 0, 319};  // Default calibration for 240x320 display
        lcd->setTouchCalibrate(defaultCalData);
        Serial.println("Default touch calibration applied - calibrate from Settings if needed");
    }
    Serial.println("TouchManager initialized with LovyanGFX hardware SPI touch");
#endif

    initialized = true;
    return true;
}

void TouchManager::update() {
    if (!initialized || !lcd) {
        return;
    }

    // If calibrating, let LovyanGFX handle it
    if (calibrating) {
        return;
    }

    // Get current touch state
    currentTouch = getTouchCoordinates();

    // Detect new tap with debouncing
    if (currentTouch.pressed && !lastTouch.pressed) {
        if (millis() - lastTouchTime > debounceTime) {
            tapped = true;
            lastTouchTime = millis();
            // Reduced debug output - only show when we detect a new tap
            Serial.printf("New tap at (%d, %d)\n", currentTouch.x, currentTouch.y);
        }
    } else if (!currentTouch.pressed) {
        tapped = false;
    }

    lastTouch = currentTouch;
}

bool TouchManager::isTouchedInArea(int x, int y, int width, int height) {
    if (!currentTouch.pressed) return false;

    return (currentTouch.x >= x && currentTouch.x < x + width &&
            currentTouch.y >= y && currentTouch.y < y + height);
}

bool TouchManager::wasTappedInArea(int x, int y, int width, int height) {
    if (!tapped) return false;

    return (currentTouch.x >= x && currentTouch.x < x + width &&
            currentTouch.y >= y && currentTouch.y < y + height);
}

bool TouchManager::needsCalibration() {
    // Only return true if we want to force recalibration
    // Normal operation should work with default values
    return false;
}

void TouchManager::startCalibration() {
    Serial.println("Starting touch calibration...");
    performTouchCalibration();
}

void TouchManager::resetCalibration() {
    clearTouchCalibration();
    performTouchCalibration();
}

TouchManager::TouchPoint TouchManager::getTouchCoordinates() {
#ifdef USE_BITBANG_TOUCH
    return getBitbangTouchCoordinates();
#else
    TouchPoint point;

    int pos[2] = {0, 0};
    bool isTouched = lcd->getTouch(&pos[0], &pos[1]);

    if (isTouched) {
        // Clamp coordinates to screen boundaries
        point.x = constrain(pos[0], 0, lcd->width() - 1);
        point.y = constrain(pos[1], 0, lcd->height() - 1);
        point.pressed = true;

        // Only show debug output when coordinates are actually clamped
        if (pos[0] != point.x || pos[1] != point.y) {
            Serial.printf("Touch clamped: (%d, %d) -> (%d, %d)\n",
                          pos[0], pos[1], point.x, point.y);
        }
    }

    return point;
#endif
}

bool TouchManager::hasSavedCalibration() {
    preferences.begin("touch_cal", true);  // Read-only mode
    bool hasValid = preferences.getBool("cal_valid", false);
    preferences.end();
    return hasValid;
}

bool TouchManager::saveTouchCalibration(std::uint16_t* calData) {
    if (calData == nullptr) {
        Serial.println("Error: Invalid calibration data");
        return false;
    }

    preferences.begin("touch_cal", false);

    // Save calibration data as bytes
    size_t bytesWritten = preferences.putBytes("cal_data", calData, 8 * sizeof(std::uint16_t));
    bool success = (bytesWritten == 8 * sizeof(std::uint16_t));

    if (success) {
        // Set a flag to indicate we have valid calibration data
        preferences.putBool("cal_valid", true);

        // Log the calibration values for debugging
        Serial.print("Saved calibration data: ");
        for (int i = 0; i < 8; i++) {
            Serial.print(calData[i]);
            if (i < 7) Serial.print(", ");
        }
        Serial.println();
    } else {
        Serial.println("Error: Failed to save calibration data to preferences");
    }

    preferences.end();
    return success;
}

bool TouchManager::loadTouchCalibration(std::uint16_t* calData) {
    if (calData == nullptr) {
        Serial.println("Error: Invalid calibration data buffer");
        return false;
    }

    preferences.begin("touch_cal", true);  // Read-only mode

    // Check if we have valid calibration data
    bool hasValidData = preferences.getBool("cal_valid", false);
    if (!hasValidData) {
        preferences.end();
        return false;
    }

    // Load calibration data
    size_t bytesRead = preferences.getBytes("cal_data", calData, 8 * sizeof(std::uint16_t));
    bool success = (bytesRead == 8 * sizeof(std::uint16_t));

    if (success) {
        // Log the loaded calibration values for debugging
        Serial.print("Loaded calibration data: ");
        for (int i = 0; i < 8; i++) {
            Serial.print(calData[i]);
            if (i < 7) Serial.print(", ");
        }
        Serial.println();
    } else {
        Serial.println("Error: Failed to load calibration data from preferences");
    }

    preferences.end();
    return success;
}

void TouchManager::performTouchCalibration() {
    if (!lcd) return;

    calibrating = true;

    // Clear screen with yellow background for calibration
    lcd->fillScreen(0xFFE0);  // Yellow

    lcd->setTextColor(0x0000);  // Black text
    lcd->setTextSize(2);
    lcd->setCursor(UIScale::scale(30), UIScale::scale(110));
    lcd->println("TOUCH");
    lcd->setCursor(UIScale::scale(30), UIScale::scale(130));
    lcd->println("CALIBRATION");

    // Use LovyanGFX's built-in calibration system
    std::uint16_t fg = 0xFFFF;  // White
    std::uint16_t bg = 0x0000;  // Black

    // For E-Paper displays, swap colors
    if (lcd->isEPD()) {
        std::swap(fg, bg);
    }

    std::uint16_t calData[8];  // Array to store calibration data
    lcd->calibrateTouch(calData, fg, bg, std::max(lcd->width(), lcd->height()) >> 3);

    // Save the calibration data to preferences
    if (saveTouchCalibration(calData)) {
        Serial.println("Touch calibration saved to storage");
    } else {
        Serial.println("Failed to save touch calibration");
    }

    calibrating = false;

    // Clear screen after calibration
    lcd->fillScreen(0x0000);  // Black
}

void TouchManager::clearTouchCalibration() {
    preferences.begin("touch_cal", false);
    preferences.clear();
    preferences.end();
    Serial.println("Touch calibration data cleared");
}

void TouchManager::showTouchDebugInfo() {
    if (!initialized || !lcd) {
        Serial.println("TouchManager not initialized");
        return;
    }

    Serial.println("=== Touch Debug Info ===");
    Serial.printf("Display dimensions: %dx%d\n", lcd->width(), lcd->height());
    Serial.printf("Current touch: (%d, %d) pressed=%s\n",
                  currentTouch.x, currentTouch.y, currentTouch.pressed ? "true" : "false");
    Serial.printf("Last touch: (%d, %d) pressed=%s\n",
                  lastTouch.x, lastTouch.y, lastTouch.pressed ? "true" : "false");
    Serial.printf("Tapped: %s\n", tapped ? "true" : "false");
    Serial.printf("Calibrating: %s\n", calibrating ? "true" : "false");
#ifdef USE_BITBANG_TOUCH
    Serial.printf("Using software SPI touch (bitbang)\n");
#else
    Serial.printf("Using LovyanGFX hardware SPI touch\n");
#endif
    Serial.println("========================");
}

#ifdef USE_BITBANG_TOUCH
bool TouchManager::initializeBitbangTouch() {
    if (touchController) {
        delete touchController;
    }

    touchController = new XPT2046_Bitbang(TOUCH_MOSI, TOUCH_MISO, TOUCH_SCK, TOUCH_CS);
    touchController->begin();
    // touchController->setRotation(1);  // Match display rotation

    Serial.println("Software SPI touch controller initialized");
    return true;
}

TouchManager::TouchPoint TouchManager::getBitbangTouchCoordinates() {
    TouchPoint point;

    if (!touchController) {
        return point;
    }

    auto touch = touchController->getTouch();

    if (touch.zRaw >= 500) {  // Minimum pressure threshold
        // Map raw coordinates to screen coordinates
        // These values may need calibration for your specific display
        point.x = map(touch.xRaw, 200, 3700, 0, lcd->width() - 1);
        point.y = map(touch.yRaw, 240, 3800, 0, lcd->height() - 1);
        point.pressed = true;

        // Clamp to screen boundaries
        point.x = constrain(point.x, 0, lcd->width() - 1);
        point.y = constrain(point.y, 0, lcd->height() - 1);
    }

    return point;
}
#endif

}  // namespace UI
}  // namespace BTLogger