#include "TouchManager.hpp"
#include "UIScale.hpp"

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
int TouchManager::touchThreshold = 10;

bool TouchManager::calibrating = false;
int TouchManager::calibrationStep = 0;
TouchManager::CalibrationData TouchManager::calibrationData;
TouchManager::TouchPoint TouchManager::calibrationPoints[4];
TouchManager::TouchPoint TouchManager::rawCalibrationPoints[4];

bool TouchManager::initialize(lgfx::LGFX_Device& display) {
    if (initialized) {
        return true;
    }

    lcd = &display;
    preferences.begin("touch-cal", false);

    // Set up calibration points (corners)
    calibrationPoints[0] = TouchPoint(20, 20);    // Top-left
    calibrationPoints[1] = TouchPoint(220, 20);   // Top-right
    calibrationPoints[2] = TouchPoint(220, 300);  // Bottom-right
    calibrationPoints[3] = TouchPoint(20, 300);   // Bottom-left

    loadCalibration();

    initialized = true;
    Serial.println("TouchManager initialized");
    return true;
}

void TouchManager::update() {
    if (!initialized || !lcd) {
        return;
    }

    if (calibrating) {
        TouchPoint raw = readRawTouch();
        if (raw.pressed) {
            processCalibrationTouch(raw);
        }
        return;
    }

    TouchPoint raw = readRawTouch();

    if (raw.pressed) {
        currentTouch = transformTouch(raw);
        currentTouch.pressed = true;

        // Detect new tap
        if (!lastTouch.pressed) {
            tapped = true;
            lastTouchTime = millis();
        }
    } else {
        currentTouch.pressed = false;
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
    return !calibrationData.valid;
}

void TouchManager::startCalibration() {
    calibrating = true;
    calibrationStep = 0;
    drawCalibrationScreen();
}

void TouchManager::resetCalibration() {
    calibrationData.valid = false;
    preferences.putBool("valid", false);
    startCalibration();
}

TouchManager::TouchPoint TouchManager::readRawTouch() {
    TouchPoint point;

    uint16_t x, y;
    if (lcd->getTouch(&x, &y)) {
        point.x = x;
        point.y = y;
        point.pressed = true;
    }

    return point;
}

TouchManager::TouchPoint TouchManager::transformTouch(const TouchPoint& raw) {
    if (!calibrationData.valid) {
        // Use default scaling
        TouchPoint transformed;
        transformed.x = map(raw.x, 0, 4095, 0, 240);
        transformed.y = map(raw.y, 0, 4095, 0, 320);
        transformed.pressed = raw.pressed;
        return transformed;
    }

    // Apply calibration transformation
    TouchPoint transformed;
    transformed.x = map(raw.x, calibrationData.x_min, calibrationData.x_max, 0, 240);
    transformed.y = map(raw.y, calibrationData.y_min, calibrationData.y_max, 0, 320);
    transformed.pressed = raw.pressed;

    return transformed;
}

void TouchManager::saveCalibration() {
    preferences.putInt("x_min", calibrationData.x_min);
    preferences.putInt("x_max", calibrationData.x_max);
    preferences.putInt("y_min", calibrationData.y_min);
    preferences.putInt("y_max", calibrationData.y_max);
    preferences.putBool("valid", calibrationData.valid);
}

void TouchManager::loadCalibration() {
    calibrationData.x_min = preferences.getInt("x_min", 0);
    calibrationData.x_max = preferences.getInt("x_max", 240);
    calibrationData.y_min = preferences.getInt("y_min", 0);
    calibrationData.y_max = preferences.getInt("y_max", 320);
    calibrationData.valid = preferences.getBool("valid", false);
}

void TouchManager::drawCalibrationScreen() {
    if (!lcd) return;

    lcd->fillScreen(0x0000);    // Black background
    lcd->setTextColor(0xFFFF);  // White text
    lcd->setTextSize(2);

    lcd->setCursor(20, 100);
    lcd->print("Touch Calibration");
    lcd->setCursor(20, 130);
    lcd->printf("Step %d of 4", calibrationStep + 1);
    lcd->setCursor(20, 160);
    lcd->print("Touch the cross");

    drawCalibrationPoint(calibrationPoints[calibrationStep].x,
                         calibrationPoints[calibrationStep].y, true);
}

void TouchManager::drawCalibrationPoint(int x, int y, bool active) {
    if (!lcd) return;

    uint16_t color = active ? 0xF800 : 0xFFFF;  // Red if active, white if not

    // Draw cross
    lcd->drawLine(x - 10, y, x + 10, y, color);
    lcd->drawLine(x, y - 10, x, y + 10, color);
    lcd->fillCircle(x, y, 3, color);
}

void TouchManager::processCalibrationTouch(const TouchPoint& raw) {
    static unsigned long lastCalTouch = 0;

    if (millis() - lastCalTouch < 500) return;  // Debounce

    rawCalibrationPoints[calibrationStep] = raw;
    calibrationStep++;

    if (calibrationStep >= 4) {
        calculateCalibration();
        calibrating = false;
        lcd->fillScreen(0x0000);  // Clear screen
    } else {
        drawCalibrationScreen();
    }

    lastCalTouch = millis();
}

void TouchManager::calculateCalibration() {
    // Simple linear calibration using corner points
    calibrationData.x_min = (rawCalibrationPoints[0].x + rawCalibrationPoints[3].x) / 2;
    calibrationData.x_max = (rawCalibrationPoints[1].x + rawCalibrationPoints[2].x) / 2;
    calibrationData.y_min = (rawCalibrationPoints[0].y + rawCalibrationPoints[1].y) / 2;
    calibrationData.y_max = (rawCalibrationPoints[2].y + rawCalibrationPoints[3].y) / 2;

    calibrationData.valid = true;
    saveCalibration();

    Serial.println("Touch calibration complete");
    Serial.printf("X: %d-%d, Y: %d-%d\n",
                  calibrationData.x_min, calibrationData.x_max,
                  calibrationData.y_min, calibrationData.y_max);
}

}  // namespace UI
}  // namespace BTLogger