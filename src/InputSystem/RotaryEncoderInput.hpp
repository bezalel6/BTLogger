#pragma once

#include "../../include/InputSystem.hpp"
#include "ButtonInput.hpp"
#include <ESP32Encoder.h>
#include <memory>

// Rotary encoder input device implementation
class RotaryEncoderInput : public InputDevice {
   public:
    RotaryEncoderInput(uint8_t id, const EncoderConfig& config);
    virtual ~RotaryEncoderInput() = default;

    // InputDevice interface implementation
    void begin() override;
    void update() override;

    // Encoder-specific methods
    int32_t getValue() const;
    int32_t getDelta() const;
    void setValue(int32_t value);
    void resetValue();

    // Range methods
    void setRange(int32_t min, int32_t max);
    int32_t getMinValue() const { return config.minValue; }
    int32_t getMaxValue() const { return config.maxValue; }

    // Button methods (if encoder has button)
    bool hasButton() const { return config.buttonPin != 255; }
    bool isButtonPressed() const;

    // Configuration methods
    void setStepsPerNotch(int32_t steps) { stepsPerNotch = steps; }
    int32_t getStepsPerNotch() const { return stepsPerNotch; }

   private:
    EncoderConfig config;
    ESP32Encoder encoder;
    std::unique_ptr<ButtonInput> encoderButton;

    // State tracking
    int32_t currentValue;
    int32_t previousValue;
    int32_t lastRawValue;
    int32_t stepsPerNotch;

    // Processing methods
    void processEncoderChanges();
    void processButtonEvents();
    int32_t constrainValue(int32_t value) const;

    // Button event forwarding
    void onButtonEvent(const InputEvent& event);
};