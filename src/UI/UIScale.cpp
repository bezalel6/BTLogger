#include "UIScale.hpp"

namespace BTLogger {
namespace UI {

// Static member definitions
bool UIScale::initialized = false;
float UIScale::currentScale = 0.8f;
Preferences UIScale::preferences;

// Scale constants
const float UIScale::SCALE_TINY = 0.8f;
const float UIScale::SCALE_SMALL = 0.9f;
const float UIScale::SCALE_NORMAL = 1.3f;
const float UIScale::SCALE_LARGE = 1.5f;
const float UIScale::SCALE_HUGE = 1.8f;

void UIScale::initialize() {
    if (initialized) {
        return;
    }

    preferences.begin("ui-scale", false);
    loadScale();
    initialized = true;

    Serial.printf("UI Scale initialized: %.1fx (%s)\n", currentScale, getScaleDescription().c_str());
}

void UIScale::setScale(float scale) {
    currentScale = clampScale(scale);
    saveScale();
    Serial.printf("UI Scale set to: %.1fx (%s)\n", currentScale, getScaleDescription().c_str());
}

void UIScale::incrementScale() {
    if (currentScale < SCALE_HUGE) {
        if (currentScale < SCALE_SMALL) {
            setScale(SCALE_SMALL);
        } else if (currentScale < SCALE_NORMAL) {
            setScale(SCALE_NORMAL);
        } else if (currentScale < SCALE_LARGE) {
            setScale(SCALE_LARGE);
        } else {
            setScale(SCALE_HUGE);
        }
    }
}

void UIScale::decrementScale() {
    if (currentScale > SCALE_TINY) {
        if (currentScale > SCALE_LARGE) {
            setScale(SCALE_LARGE);
        } else if (currentScale > SCALE_NORMAL) {
            setScale(SCALE_NORMAL);
        } else if (currentScale > SCALE_SMALL) {
            setScale(SCALE_SMALL);
        } else {
            setScale(SCALE_TINY);
        }
    }
}

void UIScale::resetScale() {
    setScale(SCALE_NORMAL);
}

String UIScale::getScaleDescription() {
    if (currentScale <= SCALE_TINY) return "Tiny";
    if (currentScale <= SCALE_SMALL) return "Small";
    if (currentScale <= SCALE_NORMAL) return "Normal";
    if (currentScale <= SCALE_LARGE) return "Large";
    return "Huge";
}

void UIScale::saveScale() {
    if (initialized) {
        preferences.putFloat("scale", currentScale);
    }
}

void UIScale::loadScale() {
    currentScale = preferences.getFloat("scale", SCALE_NORMAL);
    currentScale = clampScale(currentScale);
}

float UIScale::clampScale(float scale) {
    if (scale < SCALE_TINY) return SCALE_TINY;
    if (scale > SCALE_HUGE) return SCALE_HUGE;
    return scale;
}

}  // namespace UI
}  // namespace BTLogger