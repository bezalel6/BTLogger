#pragma once

#include "InputSystem.hpp"

/**
 * @file InputExamples.hpp
 * @brief Comprehensive examples for using the Input System
 *
 * This file contains example configurations and usage patterns for the Input System.
 * The system supports buttons with debouncing, long press, double-click detection,
 * and rotary encoders with optional built-in buttons.
 */

// Example: Basic button setup
void setupBasicButton() {
    // Create a button on pin 25 with default settings (pullup, 50ms debounce, 1000ms long press)
    ButtonConfig basicButton(25);
    uint8_t buttonId = INPUT_SYSTEM.addButton(basicButton);

    Serial.printf("Basic button added with ID: %d\n", buttonId);
}

// Example: Custom button configuration
void setupCustomButton() {
    // Custom button: pin 26, no pullup, 30ms debounce, 2000ms long press, 500ms double-click window
    ButtonConfig customButton(26, false, 30, 2000, 500);
    uint8_t buttonId = INPUT_SYSTEM.addButton(customButton);

    Serial.printf("Custom button added with ID: %d\n", buttonId);
}

// Example: Basic rotary encoder
void setupBasicEncoder() {
    // Basic encoder on pins 32 (A) and 33 (B), no button
    EncoderConfig basicEncoder(32, 33);
    uint8_t encoderId = INPUT_SYSTEM.addRotaryEncoder(basicEncoder);

    Serial.printf("Basic encoder added with ID: %d\n", encoderId);
}

// Example: Rotary encoder with button and range limits
void setupAdvancedEncoder() {
    // Advanced encoder: pins 34 (A), 35 (B), button on pin 36, range 0-100, start at 50
    EncoderConfig advancedEncoder(34, 35, 36, true, 0, 100, 50);
    uint8_t encoderId = INPUT_SYSTEM.addRotaryEncoder(advancedEncoder);

    Serial.printf("Advanced encoder added with ID: %d\n", encoderId);
}

// Example: Global event handler
void setupGlobalEventHandler() {
    INPUT_SYSTEM.setGlobalCallback([](const InputEvent& event) {
        switch (event.type) {
            case InputEventType::BUTTON_PRESS:
                Serial.printf("Button %d pressed\n", event.deviceId);
                break;

            case InputEventType::BUTTON_RELEASE:
                Serial.printf("Button %d released\n", event.deviceId);
                break;

            case InputEventType::BUTTON_CLICK:
                Serial.printf("Button %d clicked\n", event.deviceId);
                break;

            case InputEventType::BUTTON_DOUBLE_CLICK:
                Serial.printf("Button %d double-clicked\n", event.deviceId);
                break;

            case InputEventType::BUTTON_LONG_PRESS:
                Serial.printf("Button %d long-pressed\n", event.deviceId);
                break;

            case InputEventType::ENCODER_CLOCKWISE:
                Serial.printf("Encoder %d turned clockwise, value: %ld\n", event.deviceId, event.value);
                break;

            case InputEventType::ENCODER_COUNTERCLOCKWISE:
                Serial.printf("Encoder %d turned counterclockwise, value: %ld\n", event.deviceId, event.value);
                break;

            case InputEventType::ENCODER_BUTTON_PRESS:
                Serial.printf("Encoder %d button pressed\n", event.deviceId);
                break;

            case InputEventType::ENCODER_BUTTON_RELEASE:
                Serial.printf("Encoder %d button released\n", event.deviceId);
                break;
        }
    });
}

// Example: Device-specific event handlers
void setupDeviceSpecificHandlers() {
    // Assume we have button ID 0 and encoder ID 1
    uint8_t buttonId = 0;
    uint8_t encoderId = 1;

    // Button-specific handler
    INPUT_SYSTEM.setDeviceCallback(buttonId, [](const InputEvent& event) {
        if (event.type == InputEventType::BUTTON_LONG_PRESS) {
            Serial.println("Special button long press action!");
            // Perform special action
        }
    });

    // Encoder-specific handler
    INPUT_SYSTEM.setDeviceCallback(encoderId, [](const InputEvent& event) {
        if (event.type == InputEventType::ENCODER_CLOCKWISE) {
            Serial.printf("Volume up! New level: %ld\n", event.value);
        } else if (event.type == InputEventType::ENCODER_COUNTERCLOCKWISE) {
            Serial.printf("Volume down! New level: %ld\n", event.value);
        }
    });
}

// Example: Complete setup function for a mixer project
void setupMixerInputs() {
    Serial.println("Setting up UniMixTouch Input System...");

    // Add multiple buttons for channel mute/solo
    for (int i = 0; i < 8; i++) {
        ButtonConfig channelButton(20 + i);  // Pins 20-27
        uint8_t buttonId = INPUT_SYSTEM.addButton(channelButton);

        // Set individual handlers for each channel
        INPUT_SYSTEM.setDeviceCallback(buttonId, [i](const InputEvent& event) {
            switch (event.type) {
                case InputEventType::BUTTON_CLICK:
                    Serial.printf("Channel %d mute toggled\n", i + 1);
                    break;
                case InputEventType::BUTTON_DOUBLE_CLICK:
                    Serial.printf("Channel %d solo toggled\n", i + 1);
                    break;
                case InputEventType::BUTTON_LONG_PRESS:
                    Serial.printf("Channel %d reset to default\n", i + 1);
                    break;
            }
        });
    }

    // Add rotary encoders for channel gains
    for (int i = 0; i < 4; i++) {
        // Each encoder uses 2 pins, starting from pin 2
        EncoderConfig gainEncoder(2 + i * 2, 3 + i * 2, 255, true, 0, 127, 64);
        uint8_t encoderId = INPUT_SYSTEM.addRotaryEncoder(gainEncoder);

        INPUT_SYSTEM.setDeviceCallback(encoderId, [i](const InputEvent& event) {
            if (event.type == InputEventType::ENCODER_CLOCKWISE ||
                event.type == InputEventType::ENCODER_COUNTERCLOCKWISE) {
                Serial.printf("Channel %d gain: %ld\n", i + 1, event.value);
                // Update actual gain value in your mixer code
            }
        });
    }

    // Master volume encoder with button (pins 10, 11, button on 12)
    EncoderConfig masterVolume(10, 11, 12, true, 0, 100, 75);
    uint8_t masterEncoderId = INPUT_SYSTEM.addRotaryEncoder(masterVolume);

    INPUT_SYSTEM.setDeviceCallback(masterEncoderId, [](const InputEvent& event) {
        switch (event.type) {
            case InputEventType::ENCODER_CLOCKWISE:
            case InputEventType::ENCODER_COUNTERCLOCKWISE:
                Serial.printf("Master volume: %ld%%\n", event.value);
                break;
            case InputEventType::BUTTON_CLICK:
                Serial.println("Master mute toggled");
                break;
            case InputEventType::BUTTON_LONG_PRESS:
                Serial.println("Reset all channels to default");
                break;
        }
    });

    Serial.println("Input system setup complete!");
}

// Example: Runtime control functions
void demonstrateRuntimeControl() {
    // Print system status
    INPUT_SYSTEM.printDeviceInfo();

    // Disable a specific device (useful for mode switching)
    INPUT_SYSTEM.disableDevice(0);

    // Re-enable it later
    INPUT_SYSTEM.enableDevice(0);

    // Check if device is enabled
    if (INPUT_SYSTEM.isDeviceEnabled(0)) {
        Serial.println("Device 0 is active");
    }
}