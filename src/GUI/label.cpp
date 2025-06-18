#include "label.hpp"

void Label::draw(LGFX lcd) {
    // Only draw if the label needs redrawing
    if (!needsRedraw) {
        return;
    }

    // Set text size
    lcd.setTextSize(textSize);

    // Draw background if specified
    if (hasBackground) {
        lcd.fillRect(bounds.origin.x, bounds.origin.y, bounds.w, bounds.h, backgroundColor);
    }

    // Calculate text dimensions
    int16_t textWidth = lcd.textWidth(this->text);
    int16_t textHeight = lcd.fontHeight();

    // Calculate position based on alignment
    int16_t textX = bounds.origin.x;
    int16_t textY = bounds.origin.y;

    switch (alignment) {
        case Alignment::START:
            textX = bounds.origin.x;
            break;
        case Alignment::CENTER:
            textX = bounds.origin.x + (bounds.w - textWidth) / 2;
            break;
        case Alignment::END:
            textX = bounds.origin.x + bounds.w - textWidth;
            break;
        case Alignment::STRETCH:
            textX = bounds.origin.x;
            break;
    }

    // Vertically center the text
    textY = bounds.origin.y + (bounds.h - textHeight) / 2;

    // Set text color
    if (hasBackground) {
        lcd.setTextColor(textColor, backgroundColor);
    } else {
        lcd.setTextColor(textColor);
    }

    // Draw the text
    lcd.setCursor(textX, textY);
    lcd.print(this->text);

    // Mark as clean since we just drew it
    markClean();
}