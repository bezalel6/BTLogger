#include "UIScale.hpp"

namespace BTLogger {
namespace UI {

// Scale constants
const float UIScale::SCALE_TINY = 0.8f;
const float UIScale::SCALE_SMALL = 0.9f;
const float UIScale::SCALE_NORMAL = 1.0f;
const float UIScale::SCALE_LARGE = 1.2f;
const float UIScale::SCALE_HUGE = 1.5f;

// Default values
const float UIScale::DEFAULT_SCALE = SCALE_NORMAL;
const int UIScale::DEFAULT_LABEL_TEXT_SIZE = 1;
const int UIScale::DEFAULT_BUTTON_TEXT_SIZE = 2;
const int UIScale::DEFAULT_GENERAL_TEXT_SIZE = 1;

// Font size estimation (based on standard 6x8 pixel font at size 1)
const int UIScale::CHAR_WIDTH_SIZE_1 = 6;
const int UIScale::CHAR_HEIGHT_SIZE_1 = 8;

// Static member definitions
bool UIScale::initialized = false;
float UIScale::currentScale = DEFAULT_SCALE;
int UIScale::labelTextSize = DEFAULT_LABEL_TEXT_SIZE;
int UIScale::buttonTextSize = DEFAULT_BUTTON_TEXT_SIZE;
int UIScale::generalTextSize = DEFAULT_GENERAL_TEXT_SIZE;
Preferences UIScale::preferences;

void UIScale::initialize() {
    if (initialized) {
        return;
    }

    loadSettings();
    initialized = true;
    Serial.printf("UIScale initialized - Scale: %.1fx, Label: %d, Button: %d, General: %d\n",
                  currentScale, labelTextSize, buttonTextSize, generalTextSize);
}

void UIScale::setScale(float scale) {
    float newScale = clampScale(scale);
    if (newScale != currentScale) {
        currentScale = newScale;
        saveScale();
        Serial.printf("UI Scale changed to %.1fx\n", currentScale);
    }
}

void UIScale::incrementScale() {
    if (currentScale < SCALE_TINY + 0.05f) {
        setScale(SCALE_SMALL);
    } else if (currentScale < SCALE_SMALL + 0.05f) {
        setScale(SCALE_NORMAL);
    } else if (currentScale < SCALE_NORMAL + 0.05f) {
        setScale(SCALE_LARGE);
    } else if (currentScale < SCALE_LARGE + 0.05f) {
        setScale(SCALE_HUGE);
    }
}

void UIScale::decrementScale() {
    if (currentScale > SCALE_HUGE - 0.05f) {
        setScale(SCALE_LARGE);
    } else if (currentScale > SCALE_LARGE - 0.05f) {
        setScale(SCALE_NORMAL);
    } else if (currentScale > SCALE_NORMAL - 0.05f) {
        setScale(SCALE_SMALL);
    } else if (currentScale > SCALE_SMALL - 0.05f) {
        setScale(SCALE_TINY);
    }
}

void UIScale::resetScale() {
    setScale(DEFAULT_SCALE);
}

// Text sizing methods
void UIScale::setLabelTextSize(int size) {
    labelTextSize = constrain(size, 1, 4);
    saveSettings();
    Serial.printf("Label text size set to %d\n", labelTextSize);
}

void UIScale::setButtonTextSize(int size) {
    buttonTextSize = constrain(size, 1, 4);
    saveSettings();
    Serial.printf("Button text size set to %d\n", buttonTextSize);
}

void UIScale::setGeneralTextSize(int size) {
    generalTextSize = constrain(size, 1, 4);
    saveSettings();
    Serial.printf("General text size set to %d\n", generalTextSize);
}

int UIScale::getLabelTextSize() {
    return labelTextSize;
}

int UIScale::getButtonTextSize() {
    return buttonTextSize;
}

int UIScale::getGeneralTextSize() {
    return generalTextSize;
}

// Text width calculation helpers
int UIScale::calculateTextWidth(const String& text, int textSize) {
    // Estimate text width based on character count and font size
    int charCount = text.length();
    int charWidth = CHAR_WIDTH_SIZE_1 * textSize;
    return charCount * charWidth;
}

int UIScale::calculateTextHeight(int textSize) {
    return CHAR_HEIGHT_SIZE_1 * textSize;
}

// Settings persistence
void UIScale::saveSettings() {
    if (!initialized) return;

    preferences.begin("ui_scale", false);
    preferences.putFloat("scale", currentScale);
    preferences.putInt("label_text", labelTextSize);
    preferences.putInt("button_text", buttonTextSize);
    preferences.putInt("general_text", generalTextSize);
    preferences.end();
}

void UIScale::loadSettings() {
    preferences.begin("ui_scale", true);
    currentScale = preferences.getFloat("scale", DEFAULT_SCALE);
    labelTextSize = preferences.getInt("label_text", DEFAULT_LABEL_TEXT_SIZE);
    buttonTextSize = preferences.getInt("button_text", DEFAULT_BUTTON_TEXT_SIZE);
    generalTextSize = preferences.getInt("general_text", DEFAULT_GENERAL_TEXT_SIZE);
    preferences.end();

    // Clamp values to valid ranges
    currentScale = clampScale(currentScale);
    labelTextSize = constrain(labelTextSize, 1, 4);
    buttonTextSize = constrain(buttonTextSize, 1, 4);
    generalTextSize = constrain(generalTextSize, 1, 4);
}

String UIScale::getScaleDescription() {
    if (currentScale <= SCALE_TINY + 0.05f) return "Tiny (0.8x)";
    if (currentScale <= SCALE_SMALL + 0.05f) return "Small (0.9x)";
    if (currentScale <= SCALE_NORMAL + 0.05f) return "Normal (1.0x)";
    if (currentScale <= SCALE_LARGE + 0.05f) return "Large (1.2x)";
    return "Huge (1.5x)";
}

void UIScale::saveScale() {
    if (!initialized) return;

    preferences.begin("ui_scale", false);
    preferences.putFloat("scale", currentScale);
    preferences.end();
}

void UIScale::loadScale() {
    preferences.begin("ui_scale", true);
    currentScale = preferences.getFloat("scale", DEFAULT_SCALE);
    preferences.end();
    currentScale = clampScale(currentScale);
}

float UIScale::clampScale(float scale) {
    return constrain(scale, SCALE_TINY, SCALE_HUGE);
}

}  // namespace UI
}  // namespace BTLogger