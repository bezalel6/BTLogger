#pragma once
#include "../utils.hpp"
#include "ESP32_SPI_9341.h"
#include "layout.hpp"

class Component {
   public:
    Rectangle bounds;
    bool needsRedraw;
    LayoutProps layoutProps;

    // Parent panel reference for layout calculations
    class Panel* parentPanel = nullptr;

    Component(Rectangle& rect) {
        this->bounds = rect;
        this->needsRedraw = true;  // Initially needs to be drawn
    }

    // Constructor for layout-managed components (no absolute bounds)
    Component() {
        this->bounds = Rectangle(0, 0, 0, 0);
        this->needsRedraw = true;
    }
    virtual void draw(LGFX lcd) = 0;

    // Virtual method to check component type (replaces dynamic_cast)
    virtual bool isPanel() const { return false; }

    void markDirty() {
        needsRedraw = true;
    }

    void markClean() {
        needsRedraw = false;
    }

    bool checkTouching(const TouchCoordinates& touchCoords) {
        // Global debouncing check
        static unsigned long lastTouchTime = 0;
        static const unsigned long DEBOUNCE_TIME_MS = 200;  // 200ms debounce

        unsigned long currentTime = millis();
        if (currentTime - lastTouchTime < DEBOUNCE_TIME_MS) {
            return false;
        }

        if (!touchCoords.isTouched) {
            this->isDebouncing = false;
            return false;
        }

        if (this->isDebouncing) {
            return false;
        }

        // Check bounds with minimal debug output
        auto didTouch = this->bounds.checkInside(touchCoords.toPoint());

        if (didTouch) {
            lastTouchTime = currentTime;
            this->isDebouncing = true;
        }

        return didTouch;
    }

    void clicked() {
        if (this->onClick) {
            this->onClick();
        }
    };

    // Reset debouncing state (called by GuiManager after handling)
    void resetDebouncing() {
        this->isDebouncing = false;
    }

    //    private:
    f_void onClick;

   private:
    bool isDebouncing = false;
};