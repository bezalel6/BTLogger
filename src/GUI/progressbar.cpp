#include "progressbar.hpp"
#include <cmath>

void ProgressBar::draw(LGFX lcd) {
    // Only draw if the progress bar needs redrawing
    if (!needsRedraw) {
        return;
    }

    // Draw border
    if (borderWidth > 0) {
        lcd.fillRect(bounds.origin.x, bounds.origin.y, bounds.w, bounds.h, borderColor);
    }

    // Calculate inner bounds (excluding border)
    int innerX = bounds.origin.x + borderWidth;
    int innerY = bounds.origin.y + borderWidth;
    int innerW = bounds.w - (2 * borderWidth);
    int innerH = bounds.h - (2 * borderWidth);

    // Draw background
    lcd.fillRect(innerX, innerY, innerW, innerH, backgroundColor);

    // Calculate progress width
    float percentage = getPercentage();
    int progressW = static_cast<int>(innerW * percentage);

    // Draw progress fill
    if (progressW > 0) {
        lcd.fillRect(innerX, innerY, progressW, innerH, foregroundColor);
    }

    // Draw text if enabled
    if (showText) {
        // Format the percentage text
        String text = String(static_cast<int>(percentage * 100)) + "%";

        // Calculate text position (centered)
        int16_t textWidth = lcd.textWidth(text);
        int16_t textHeight = lcd.fontHeight();
        int16_t textX = bounds.origin.x + (bounds.w - textWidth) / 2;
        int16_t textY = bounds.origin.y + (bounds.h - textHeight) / 2;

        // Set text color and draw
        lcd.setTextColor(textColor);
        lcd.setCursor(textX, textY);
        lcd.print(text);
    }

    // Mark as clean since we just drew it
    markClean();
}

void ProgressBar::setValue(float value) {
    currentValue = value;
    constrainValue();
    markDirty();
}

void ProgressBar::setMinMax(float min, float max) {
    minValue = min;
    maxValue = max;
    constrainValue();
    markDirty();
}

float ProgressBar::getPercentage() const {
    if (maxValue <= minValue) {
        return 0.0f;
    }
    return (currentValue - minValue) / (maxValue - minValue);
}

void ProgressBar::setColors(uint16_t bg, uint16_t fg, uint16_t border) {
    backgroundColor = bg;
    foregroundColor = fg;
    borderColor = border;
    markDirty();
}

void ProgressBar::increment(float amount) {
    setValue(currentValue + amount);
}

void ProgressBar::decrement(float amount) {
    setValue(currentValue - amount);
}

void ProgressBar::constrainValue() {
    if (currentValue < minValue) {
        currentValue = minValue;
    } else if (currentValue > maxValue) {
        currentValue = maxValue;
    }
}