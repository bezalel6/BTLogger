#pragma once

#include "Arduino.h"
#include "component.hpp"

class ProgressBar : public Component {
   public:
    // Traditional constructor with fixed bounds
    ProgressBar(Rectangle rect, float minValue = 0.0f, float maxValue = 100.0f) : Component(rect) {
        this->minValue = minValue;
        this->maxValue = maxValue;
        this->currentValue = minValue;
    }

    // New constructor for layout-managed progress bars
    ProgressBar(float minValue = 0.0f, float maxValue = 100.0f) : Component() {
        this->minValue = minValue;
        this->maxValue = maxValue;
        this->currentValue = minValue;

        // Set default layout properties for progress bars
        layoutProps.setSize(200, 30);  // Default progress bar size
        layoutProps.setMargin(5);      // Default margin
    }

    void draw(LGFX lcd) override;

    // Value management
    void setValue(float value);
    void setMinMax(float min, float max);
    float getValue() const { return currentValue; }
    float getPercentage() const;

    // Styling
    void setColors(uint16_t bg, uint16_t fg, uint16_t border = TFT_WHITE);
    void setBorderWidth(int width) {
        borderWidth = width;
        markDirty();
    }
    void setShowText(bool show) {
        showText = show;
        markDirty();
    }
    void setTextColor(uint16_t color) {
        textColor = color;
        markDirty();
    }

    // Increment/decrement helpers
    void increment(float amount = 1.0f);
    void decrement(float amount = 1.0f);

   private:
    float minValue;
    float maxValue;
    float currentValue;

    // Styling
    uint16_t backgroundColor = TFT_DARKGRAY;
    uint16_t foregroundColor = TFT_GREEN;
    uint16_t borderColor = TFT_WHITE;
    uint16_t textColor = TFT_WHITE;
    int borderWidth = 1;
    bool showText = true;

    // Helper methods
    void constrainValue();
};