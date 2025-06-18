#include "RotaryEncoderInput.hpp"
#include <memory>

RotaryEncoderInput::RotaryEncoderInput(uint8_t id, const EncoderConfig& config)
    : InputDevice(id), config(config), encoder(), encoderButton(nullptr), currentValue(config.initialValue), previousValue(config.initialValue), lastRawValue(0), stepsPerNotch(4) {
}
#define ENABLE_PULLUPS true
void RotaryEncoderInput::begin() {
    // Setup pins
    if (ENABLE_PULLUPS) {
        pinMode(config.pinA, INPUT_PULLUP);
        pinMode(config.pinB, INPUT_PULLUP);
    } else {
        pinMode(config.pinA, INPUT);
        pinMode(config.pinB, INPUT);
    }

    // Initialize ESP32Encoder
    ESP32Encoder::useInternalWeakPullResistors = ENABLE_PULLUPS ? puType::up : puType::none;
    encoder.attachFullQuad(config.pinA, config.pinB);
    // encoder.clearCount();

    // // Initialize ESP32Encoder
    // encoder.attachHalfQuad(config.pinA, config.pinB);
    encoder.setCount(config.initialValue * stepsPerNotch);

    // Initialize button if configured
    if (hasButton()) {
        ButtonConfig btnConfig(config.buttonPin);
        encoderButton = std::make_unique<ButtonInput>(deviceId + 100, btnConfig);  // Offset ID for button
        encoderButton->begin();

        // Set up button event forwarding
        encoderButton->setCallback([this](const InputEvent& event) {
            this->onButtonEvent(event);
        });
    }

    // Initialize state
    lastRawValue = encoder.getCount();
    currentValue = constrainValue(config.initialValue);
    previousValue = currentValue;

    Serial.printf("[RotaryEncoderInput] Encoder %d initialized on pins A=%d, B=%d",
                  deviceId, config.pinA, config.pinB);
    if (hasButton()) {
        Serial.printf(", Button=%d", config.buttonPin);
    }
    Serial.println();
}

void RotaryEncoderInput::update() {
    if (!enabled) return;

    // Update encoder state
    processEncoderChanges();

    // Update button if present
    if (encoderButton) {
        encoderButton->update();
    }
}

void RotaryEncoderInput::processEncoderChanges() {
    int32_t rawValue = encoder.getCount();

    if (rawValue != lastRawValue) {
        // Calculate the change in raw encoder steps
        int32_t rawDelta = rawValue - lastRawValue;

        // Convert to logical steps (accounting for steps per notch)
        int32_t logicalDelta = rawDelta / stepsPerNotch;

        if (logicalDelta != 0) {
            previousValue = currentValue;
            currentValue = constrainValue(currentValue + logicalDelta);

            // Only trigger events if the value actually changed (not constrained out)
            if (currentValue != previousValue) {
                if (logicalDelta > 0) {
                    for (int32_t i = 0; i < logicalDelta; i++) {
                        triggerEvent(InputEventType::ENCODER_CLOCKWISE, currentValue);
                    }
                } else {
                    for (int32_t i = 0; i < -logicalDelta; i++) {
                        triggerEvent(InputEventType::ENCODER_COUNTERCLOCKWISE, currentValue);
                    }
                }
            }

            // Update raw value to account for processed steps
            lastRawValue = rawValue - (rawDelta % stepsPerNotch);
            encoder.setCount(lastRawValue);
        } else {
            lastRawValue = rawValue;
        }
    }
}

void RotaryEncoderInput::onButtonEvent(const InputEvent& event) {
    // Forward button events with modified type
    InputEventType newType;
    switch (event.type) {
        case InputEventType::BUTTON_PRESS:
            newType = InputEventType::ENCODER_BUTTON_PRESS;
            break;
        case InputEventType::BUTTON_RELEASE:
            newType = InputEventType::ENCODER_BUTTON_RELEASE;
            break;
        case InputEventType::BUTTON_CLICK:
        case InputEventType::BUTTON_DOUBLE_CLICK:
        case InputEventType::BUTTON_LONG_PRESS:
            // Forward these as-is since they're encoder button specific
            newType = event.type;
            break;
        default:
            return;  // Don't forward other event types
    }

    if (callback) {
        callback(InputEvent(newType, deviceId, event.value));
    }
}

int32_t RotaryEncoderInput::constrainValue(int32_t value) const {
    if (value < config.minValue) return config.minValue;
    if (value > config.maxValue) return config.maxValue;
    return value;
}

int32_t RotaryEncoderInput::getValue() const {
    return currentValue;
}

int32_t RotaryEncoderInput::getDelta() const {
    return currentValue - previousValue;
}

void RotaryEncoderInput::setValue(int32_t value) {
    int32_t newValue = constrainValue(value);
    if (newValue != currentValue) {
        previousValue = currentValue;
        currentValue = newValue;
        encoder.setCount(currentValue * stepsPerNotch);
        lastRawValue = encoder.getCount();
    }
}

void RotaryEncoderInput::resetValue() {
    setValue(config.initialValue);
}

void RotaryEncoderInput::setRange(int32_t min, int32_t max) {
    config.minValue = min;
    config.maxValue = max;
    currentValue = constrainValue(currentValue);
}

bool RotaryEncoderInput::isButtonPressed() const {
    return encoderButton ? encoderButton->isPressed() : false;
}