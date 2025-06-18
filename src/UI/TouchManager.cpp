#include "TouchManager.hpp"
#include "UIScale.hpp"
#include <algorithm>

namespace BTLogger {
namespace UI {

// Static member definitions
bool TouchManager::initialized = false;
lgfx::LGFX_Device* TouchManager::lcd = nullptr;
Preferences TouchManager::preferences;

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
        // No saved calibration found, perform calibration
        Serial.println("No saved touch calibration found, performing calibration");
        performTouchCalibration();
    }

    initialized = true;
    Serial.println("TouchManager initialized");
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

    // Detect new tap
    if (currentTouch.pressed && !lastTouch.pressed) {
        tapped = true;
        lastTouchTime = millis();
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
    return !hasSavedCalibration();
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
    TouchPoint point;

    int pos[2] = {0, 0};
    bool isTouched = lcd->getTouch(&pos[0], &pos[1]);

    if (isTouched) {
        point.x = pos[0];
        point.y = pos[1];
        point.pressed = true;
    }

    return point;
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

}  // namespace UI
}  // namespace BTLogger