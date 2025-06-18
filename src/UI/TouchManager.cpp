#include "TouchManager.hpp"
#include "UIScale.hpp"
#include <algorithm>

#ifdef USE_BITBANG_TOUCH
#include "../Hardware/ESP32_SPI_9341.h"
#endif

// Compile-time calibration version - increment this to invalidate all existing calibration data
#define TOUCH_CALIBRATION_VERSION 3

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

    // Check if we have saved calibration data for bitbang touch
    if (hasSavedCalibration()) {
        std::uint16_t calData[8];
        if (loadTouchCalibration(calData)) {
            // Extract min/max values from calibration data
            int minRawX = min(min(calData[0], calData[2]), min(calData[4], calData[6]));
            int maxRawX = max(max(calData[0], calData[2]), max(calData[4], calData[6]));
            int minRawY = min(min(calData[1], calData[3]), min(calData[5], calData[7]));
            int maxRawY = max(max(calData[1], calData[3]), max(calData[5], calData[7]));

            // Set calibration on the touch controller
            touchController->setCalibration(minRawX, maxRawX, minRawY, maxRawY);
            Serial.println("Touch calibration loaded from storage for bitbang touch");
        } else {
            Serial.println("Failed to load calibration data, performing new calibration");
            performTouchCalibration();
        }
    } else {
        Serial.println("No saved touch calibration found for bitbang touch");
        Serial.println("Starting automatic calibration...");
        performTouchCalibration();  // This will call performBitbangTouchCalibration()
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
        Serial.println("No saved touch calibration found for hardware SPI touch");
        Serial.println("Starting automatic calibration...");
        performTouchCalibration();  // This will use LovyanGFX calibration
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
    int storedVersion = preferences.getInt("cal_version", 0);
    preferences.end();

    // Calibration is valid only if the flag is set AND the version matches
    return hasValid && (storedVersion == TOUCH_CALIBRATION_VERSION);
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
        // Save the current calibration version
        preferences.putInt("cal_version", TOUCH_CALIBRATION_VERSION);

        // Log the calibration values for debugging
        Serial.print("Saved calibration data: ");
        for (int i = 0; i < 8; i++) {
            Serial.print(calData[i]);
            if (i < 7) Serial.print(", ");
        }
        Serial.printf(" (version %d)\n", TOUCH_CALIBRATION_VERSION);
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
        // Get the stored version for logging
        int storedVersion = preferences.getInt("cal_version", 0);

        // Log the loaded calibration values for debugging
        Serial.print("Loaded calibration data: ");
        for (int i = 0; i < 8; i++) {
            Serial.print(calData[i]);
            if (i < 7) Serial.print(", ");
        }
        Serial.printf(" (version %d)\n", storedVersion);
    } else {
        Serial.println("Error: Failed to load calibration data from preferences");
    }

    preferences.end();
    return success;
}

void TouchManager::performTouchCalibration() {
    if (!lcd) return;

    calibrating = true;

#ifdef USE_BITBANG_TOUCH
    performBitbangTouchCalibration();
#else
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
#endif

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
    Serial.printf("Current calibration version: %d\n", TOUCH_CALIBRATION_VERSION);

    // Check stored calibration version
    preferences.begin("touch_cal", true);
    bool hasValid = preferences.getBool("cal_valid", false);
    int storedVersion = preferences.getInt("cal_version", 0);
    preferences.end();
    Serial.printf("Stored calibration: valid=%s, version=%d\n",
                  hasValid ? "true" : "false", storedVersion);

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
        // Use the touch controller's already-calibrated coordinates
        point.x = touch.x;
        point.y = touch.y;
        point.pressed = true;

        // Clamp to screen boundaries
        point.x = constrain(point.x, 0, lcd->width() - 1);
        point.y = constrain(point.y, 0, lcd->height() - 1);
    }

    return point;
}

void TouchManager::performBitbangTouchCalibration() {
    if (!lcd || !touchController) return;

    Serial.println("Starting bitbang touch calibration...");

    // Calibration points: top-left, top-right, bottom-left, bottom-right
    struct CalibrationPoint {
        int screenX, screenY;
        int rawX, rawY;
        bool collected;
    };

    CalibrationPoint calPoints[4] = {
        {20, 20, 0, 0, false},                                // Top-left
        {lcd->width() - 20, 20, 0, 0, false},                 // Top-right
        {20, lcd->height() - 20, 0, 0, false},                // Bottom-left
        {lcd->width() - 20, lcd->height() - 20, 0, 0, false}  // Bottom-right
    };

    lcd->fillScreen(0x0000);    // Black background
    lcd->setTextColor(0xFFFF);  // White text
    lcd->setTextSize(1);

    for (int i = 0; i < 4; i++) {
        lcd->fillScreen(0x0000);

        // Draw instruction text
        lcd->setCursor(10, 10);
        lcd->printf("Calibration %d/4", i + 1);
        lcd->setCursor(10, 25);
        lcd->println("Touch crosshair center");
        lcd->setCursor(10, 40);
        lcd->println("Hold for 1 second");

        // Draw crosshair at calibration point
        drawCalibrationCrosshair(calPoints[i].screenX, calPoints[i].screenY);

        // Wait for touch on the calibration point
        bool pointCollected = false;
        unsigned long startTime = millis();
        const unsigned long timeout = 30000;  // 30 second timeout

        while (!pointCollected && (millis() - startTime) < timeout) {
            auto touch = touchController->getTouch();

            if (touch.zRaw >= 500) {  // Touch detected - accept any touch during calibration
                // Collect multiple samples for stability
                int sampleCount = 0;
                long sumX = 0, sumY = 0;
                unsigned long sampleStart = millis();

                while (millis() - sampleStart < 1000) {  // Sample for 1 second for better stability
                    auto sampleTouch = touchController->getTouch();
                    if (sampleTouch.zRaw >= 500) {
                        sumX += sampleTouch.xRaw;
                        sumY += sampleTouch.yRaw;
                        sampleCount++;
                    }
                    delay(10);
                }

                if (sampleCount > 20) {  // Need at least 20 samples for good calibration
                    calPoints[i].rawX = sumX / sampleCount;
                    calPoints[i].rawY = sumY / sampleCount;
                    calPoints[i].collected = true;
                    pointCollected = true;

                    Serial.printf("Calibration point %d: screen(%d,%d) -> raw(%d,%d) (%d samples)\n",
                                  i + 1, calPoints[i].screenX, calPoints[i].screenY,
                                  calPoints[i].rawX, calPoints[i].rawY, sampleCount);

                    // Visual feedback - fill entire screen green
                    lcd->fillScreen(0x07E0);    // Green screen
                    lcd->setTextColor(0x0000);  // Black text on green
                    lcd->setCursor(10, 100);
                    lcd->printf("Point %d accepted!", i + 1);
                    delay(1000);
                } else {
                    Serial.printf("Not enough samples (%d), touch and hold longer\n", sampleCount);
                    // Show feedback to user
                    lcd->setCursor(10, 60);
                    lcd->setTextColor(0xF800);  // Red
                    lcd->println("Hold longer!");
                    delay(1000);
                    // Redraw the crosshair and instructions
                    lcd->fillScreen(0x0000);
                    lcd->setCursor(10, 10);
                    lcd->setTextColor(0xFFFF);  // White
                    lcd->printf("Calibration %d/4", i + 1);
                    lcd->setCursor(10, 25);
                    lcd->println("Touch crosshair center");
                    lcd->setCursor(10, 40);
                    lcd->println("Hold for 1 second");
                    drawCalibrationCrosshair(calPoints[i].screenX, calPoints[i].screenY);
                }
            }
            delay(50);
        }

        if (!pointCollected) {
            Serial.println("Calibration timeout!");
            lcd->fillScreen(0x0000);
            lcd->setCursor(10, 100);
            lcd->setTextColor(0xF800);  // Red
            lcd->println("Calibration failed!");
            lcd->println("Timeout waiting for touch");
            delay(2000);
            return;
        }
    }

    // Calculate and save calibration data
    std::uint16_t calData[8] = {
        (std::uint16_t)calPoints[0].rawX, (std::uint16_t)calPoints[0].rawY,  // Top-left
        (std::uint16_t)calPoints[1].rawX, (std::uint16_t)calPoints[1].rawY,  // Top-right
        (std::uint16_t)calPoints[2].rawX, (std::uint16_t)calPoints[2].rawY,  // Bottom-left
        (std::uint16_t)calPoints[3].rawX, (std::uint16_t)calPoints[3].rawY   // Bottom-right
    };

    if (saveTouchCalibration(calData)) {
        // Extract min/max values and set calibration on the touch controller
        int minRawX = min(min(calData[0], calData[2]), min(calData[4], calData[6]));
        int maxRawX = max(max(calData[0], calData[2]), max(calData[4], calData[6]));
        int minRawY = min(min(calData[1], calData[3]), min(calData[5], calData[7]));
        int maxRawY = max(max(calData[1], calData[3]), max(calData[5], calData[7]));

        touchController->setCalibration(minRawX, maxRawX, minRawY, maxRawY);

        Serial.println("Bitbang touch calibration saved successfully");
        lcd->fillScreen(0x0000);
        lcd->setCursor(10, 100);
        lcd->setTextColor(0x07E0);  // Green
        lcd->println("Calibration");
        lcd->println("successful!");
    } else {
        Serial.println("Failed to save bitbang touch calibration");
        lcd->fillScreen(0x0000);
        lcd->setCursor(10, 100);
        lcd->setTextColor(0xF800);  // Red
        lcd->println("Failed to save");
        lcd->println("calibration!");
    }

    delay(2000);
}

void TouchManager::drawCalibrationCrosshair(int x, int y) {
    const int size = 10;
    const uint16_t color = 0xFFFF;  // White

    // Draw crosshair
    lcd->drawLine(x - size, y, x + size, y, color);
    lcd->drawLine(x, y - size, x, y + size, color);

    // Draw center circle
    lcd->fillCircle(x, y, 2, color);
}

#endif

}  // namespace UI
}  // namespace BTLogger