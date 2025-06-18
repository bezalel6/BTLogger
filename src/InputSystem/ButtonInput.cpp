#include "ButtonInput.hpp"

ButtonInput::ButtonInput(uint8_t id, const ButtonConfig& config)
    : InputDevice(id), config(config), bounce(), currentState(false), previousState(false), longPressTriggered(false), pressStartTime(0), lastClickTime(0), clickCount(0), lastUpdateTime(0), buttonState(ButtonState::IDLE) {
}

void ButtonInput::begin() {
    // Configure pin
    pinMode(config.pin, config.pullUp ? INPUT_PULLUP : INPUT);

    // Initialize Bounce library
    bounce.attach(config.pin);
    bounce.interval(config.debounceTime);

    // Initialize state
    currentState = bounce.read() == (config.pullUp ? LOW : HIGH);
    previousState = currentState;
    buttonState = ButtonState::IDLE;
    longPressTriggered = false;
    pressStartTime = 0;
    lastClickTime = 0;
    clickCount = 0;
    lastUpdateTime = millis();

    Serial.printf("[ButtonInput] Button %d initialized on pin %d\n", deviceId, config.pin);
}

void ButtonInput::update() {
    if (!enabled) return;

    unsigned long currentTime = millis();

    // Update bounce state
    bounce.update();

    // Get current button state (pressed = true)
    previousState = currentState;
    currentState = bounce.read() == (config.pullUp ? LOW : HIGH);

    // Process button events based on state changes
    processButtonEvents();

    lastUpdateTime = currentTime;
}

void ButtonInput::processButtonEvents() {
    unsigned long currentTime = millis();

    switch (buttonState) {
        case ButtonState::IDLE:
            if (currentState && !previousState) {
                // Button just pressed
                buttonState = ButtonState::PRESSED;
                pressStartTime = currentTime;
                longPressTriggered = false;
                triggerEvent(InputEventType::BUTTON_PRESS);
            }
            break;

        case ButtonState::PRESSED:
            if (!currentState && previousState) {
                // Button released
                buttonState = ButtonState::RELEASED;

                if (!longPressTriggered) {
                    // Normal release (not from long press)
                    triggerEvent(InputEventType::BUTTON_RELEASE);

                    // Check for double click timing
                    if (currentTime - lastClickTime <= config.doubleClickTime && clickCount > 0) {
                        buttonState = ButtonState::IDLE;
                        clickCount++;
                        triggerEvent(InputEventType::BUTTON_DOUBLE_CLICK);
                        resetClickTracking();
                    } else {
                        // Start waiting for potential double click
                        buttonState = ButtonState::WAIT_DOUBLE_CLICK;
                        clickCount = 1;
                        lastClickTime = currentTime;
                    }
                } else {
                    // Released from long press
                    buttonState = ButtonState::IDLE;
                    resetClickTracking();
                }
            } else if (currentState && (currentTime - pressStartTime >= config.longPressTime) && !longPressTriggered) {
                // Long press detected
                longPressTriggered = true;
                triggerEvent(InputEventType::BUTTON_LONG_PRESS);
            }
            break;

        case ButtonState::RELEASED:
            // This state is handled immediately in PRESSED case
            break;

        case ButtonState::WAIT_DOUBLE_CLICK:
            if (currentState && !previousState) {
                // Second press detected within double click window
                buttonState = ButtonState::PRESSED;
                pressStartTime = currentTime;
                longPressTriggered = false;
                triggerEvent(InputEventType::BUTTON_PRESS);
            } else if (currentTime - lastClickTime > config.doubleClickTime) {
                // Double click timeout - trigger single click
                buttonState = ButtonState::IDLE;
                triggerEvent(InputEventType::BUTTON_CLICK);
                resetClickTracking();
            }
            break;
    }
}

void ButtonInput::resetClickTracking() {
    clickCount = 0;
    lastClickTime = 0;
}

bool ButtonInput::isPressed() const {
    return currentState;
}

bool ButtonInput::wasPressed() const {
    return currentState && !previousState;
}

bool ButtonInput::wasReleased() const {
    return !currentState && previousState;
}

unsigned long ButtonInput::getPressTime() const {
    if (currentState && pressStartTime > 0) {
        return millis() - pressStartTime;
    }
    return 0;
}

uint32_t ButtonInput::getClickCount() const {
    return clickCount;
}

void ButtonInput::setDebounceTime(uint32_t ms) {
    config.debounceTime = ms;
    bounce.interval(ms);
}

void ButtonInput::setLongPressTime(uint32_t ms) {
    config.longPressTime = ms;
}

void ButtonInput::setDoubleClickTime(uint32_t ms) {
    config.doubleClickTime = ms;
}