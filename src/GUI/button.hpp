#include "Arduino.h"
#include "component.hpp"

class Button : public Component {
   public:
    // Traditional constructor with fixed bounds
    Button(Rectangle rect, String text) : Component(rect) {
        this->text = text;
        this->onClick = Button::func;
    }

    // New constructor for layout-managed buttons
    Button(String text) : Component() {
        this->text = text;
        this->onClick = Button::func;

        // Set default layout properties for buttons
        layoutProps.setSize(100, 40);  // Default button size
        layoutProps.setMargin(5);      // Default margin
    }

    static void func() {
        Serial.println("IM HERE");
    };
    void draw(LGFX lcd);

    // Setters for dynamic styling
    void setText(const String& newText) {
        text = newText;
        markDirty();
    }
    void setColors(uint16_t bg, uint16_t fg) {
        backgroundColor = bg;
        textColor = fg;
        markDirty();
    }

   private:
    String text;
    uint16_t backgroundColor = TFT_LIGHTGRAY;
    uint16_t textColor = TFT_BLACK;
};