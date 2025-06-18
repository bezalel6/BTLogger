#pragma once

#include "Arduino.h"
#include "component.hpp"

class Label : public Component {
   public:
    // Traditional constructor with fixed bounds
    Label(Rectangle rect, String text) : Component(rect) {
        this->text = text;
    }

    // New constructor for layout-managed labels
    Label(String text) : Component() {
        this->text = text;

        // Set default layout properties for labels
        layoutProps.heightMode = SizeMode::AUTO;  // Auto-size height based on content
        layoutProps.setMargin(2);                 // Small default margin
    }

    void draw(LGFX lcd) override;

    // Setters
    void setText(const String& newText) {
        text = newText;
        markDirty();
    }
    void setTextColor(uint16_t color) {
        textColor = color;
        markDirty();
    }
    void setBackgroundColor(uint16_t color) {
        backgroundColor = color;
        hasBackground = true;
        markDirty();
    }
    void clearBackground() {
        hasBackground = false;
        markDirty();
    }
    void setTextSize(int size) {
        textSize = size;
        markDirty();
    }
    void setAlignment(Alignment align) {
        alignment = align;
        markDirty();
    }

   private:
    String text;
    uint16_t textColor = TFT_WHITE;
    uint16_t backgroundColor = TFT_BLACK;
    bool hasBackground = false;
    int textSize = 2;
    Alignment alignment = Alignment::START;
};