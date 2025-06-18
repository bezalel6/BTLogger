#pragma once

#include "../../include/InputSystem.hpp"
#include <Bounce2.h>

// Button input device implementation
class ButtonInput : public InputDevice {
   public:
    ButtonInput(uint8_t id, const ButtonConfig& config);
    virtual ~ButtonInput() = default;

    // InputDevice interface implementation
    void begin() override;
    void update() override;

    // Button-specific methods
    bool isPressed() const;
    bool wasPressed() const;
    bool wasReleased() const;
    unsigned long getPressTime() const;
    uint32_t getClickCount() const;

    // Configuration methods
    void setDebounceTime(uint32_t ms);
    void setLongPressTime(uint32_t ms);
    void setDoubleClickTime(uint32_t ms);

   private:
    ButtonConfig config;
    Bounce bounce;

    // State tracking
    bool currentState;
    bool previousState;
    bool longPressTriggered;
    unsigned long pressStartTime;
    unsigned long lastClickTime;
    uint32_t clickCount;

    // Timing
    unsigned long lastUpdateTime;

    // State machine for button events
    enum class ButtonState {
        IDLE,
        PRESSED,
        RELEASED,
        WAIT_DOUBLE_CLICK,
        LONG_PRESS_WAIT
    };

    ButtonState buttonState;

    void processButtonEvents();
    void resetClickTracking();
};