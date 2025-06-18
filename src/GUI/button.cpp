#include "button.hpp"

void Button::draw(LGFX lcd) {
    // Only draw if the button needs redrawing
    if (!needsRedraw) {
        return;
    }

    // Draw button background
    lcd.fillRect(bounds.origin.x, bounds.origin.y, bounds.w, bounds.h, backgroundColor);

    // Calculate text dimensions
    int16_t textWidth = lcd.textWidth(this->text);
    int16_t textHeight = lcd.fontHeight();

    // Calculate centered position
    Point middle = bounds.getMiddle();
    int16_t textX = middle.x - (textWidth / 2);
    int16_t textY = middle.y - (textHeight / 2);

    // Set proper text colors
    lcd.setTextColor(textColor, backgroundColor);  // text color, background color
    lcd.setCursor(textX, textY);
    lcd.print(this->text);

    // Mark as clean since we just drew it
    markClean();
}
