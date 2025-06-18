#pragma once

#include <Arduino.h>
#include <vector>
#include <functional>
#include <memory>

// Forward declarations
class InputDevice;
class ButtonInput;
class RotaryEncoderInput;

// Event types for input handling
enum class InputEventType {
    BUTTON_PRESS,
    BUTTON_RELEASE,
    BUTTON_CLICK,
    BUTTON_DOUBLE_CLICK,
    BUTTON_LONG_PRESS,
    ENCODER_CLOCKWISE,
    ENCODER_COUNTERCLOCKWISE,
    ENCODER_BUTTON_PRESS,
    ENCODER_BUTTON_RELEASE
};

// Input event structure
struct InputEvent {
    InputEventType type;
    uint8_t deviceId;
    int32_t value;
    unsigned long timestamp;

    InputEvent(InputEventType t, uint8_t id, int32_t val = 0)
        : type(t), deviceId(id), value(val), timestamp(millis()) {}
};

// Callback function type for input events
using InputCallback = std::function<void(const InputEvent&)>;

// Abstract base class for all input devices
class InputDevice {
   public:
    InputDevice(uint8_t id) : deviceId(id), enabled(true) {}
    virtual ~InputDevice() = default;

    virtual void begin() = 0;
    virtual void update() = 0;
    virtual void setCallback(InputCallback callback) { this->callback = callback; }
    virtual void enable() { enabled = true; }
    virtual void disable() { enabled = false; }
    virtual bool isEnabled() const { return enabled; }
    virtual uint8_t getId() const { return deviceId; }

   protected:
    uint8_t deviceId;
    bool enabled;
    InputCallback callback;

    void triggerEvent(InputEventType type, int32_t value = 0) {
        if (enabled && callback) {
            callback(InputEvent(type, deviceId, value));
        }
    }
};

// Button configuration structure
struct ButtonConfig {
    uint8_t pin;
    bool pullUp;
    uint32_t debounceTime;
    uint32_t longPressTime;
    uint32_t doubleClickTime;

    ButtonConfig(uint8_t p, bool pu = true, uint32_t db = 50, uint32_t lp = 1000, uint32_t dc = 300)
        : pin(p), pullUp(pu), debounceTime(db), longPressTime(lp), doubleClickTime(dc) {}
};

// Rotary encoder configuration
struct EncoderConfig {
    uint8_t pinA;
    uint8_t pinB;
    uint8_t buttonPin;
    bool useInterrupts;
    int32_t minValue;
    int32_t maxValue;
    int32_t initialValue;

    EncoderConfig(uint8_t pA, uint8_t pB, uint8_t btnPin = 255, bool interrupts = true,
                  int32_t minVal = INT32_MIN, int32_t maxVal = INT32_MAX, int32_t initVal = 0)
        : pinA(pA), pinB(pB), buttonPin(btnPin), useInterrupts(interrupts), minValue(minVal), maxValue(maxVal), initialValue(initVal) {}
};

// Main Input class - Singleton pattern for global access
class Input {
   public:
    static Input& getInstance();

    // Core system methods
    void setup();
    void loop();

    // Device management
    uint8_t addButton(const ButtonConfig& config);
    uint8_t addRotaryEncoder(const EncoderConfig& config);

    // Event handling
    void setGlobalCallback(InputCallback callback);
    void setDeviceCallback(uint8_t deviceId, InputCallback callback);

    // Device control
    void enableDevice(uint8_t deviceId);
    void disableDevice(uint8_t deviceId);
    bool isDeviceEnabled(uint8_t deviceId) const;

    // Utility methods
    size_t getDeviceCount() const;
    void clearAllDevices();

    // Debug methods
    void printDeviceInfo() const;

   private:
    Input() = default;
    ~Input() = default;
    Input(const Input&) = delete;
    Input& operator=(const Input&) = delete;

    std::vector<std::unique_ptr<InputDevice>> devices;
    InputCallback globalCallback;
    uint8_t nextDeviceId = 0;
    bool initialized = false;

    void handleGlobalEvent(const InputEvent& event);
    InputDevice* getDevice(uint8_t deviceId);
};

// Convenience macros for easier usage
#define INPUT_SYSTEM Input::getInstance()
#define INPUT_SETUP() Input::getInstance().setup()
#define INPUT_LOOP() Input::getInstance().loop()