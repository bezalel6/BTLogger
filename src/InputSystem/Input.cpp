#include "../../include/InputSystem.hpp"
#include "ButtonInput.hpp"
#include "RotaryEncoderInput.hpp"

Input& Input::getInstance() {
    static Input instance;
    return instance;
}

void Input::setup() {
    if (initialized) {
        Serial.println("[Input] System already initialized!");
        return;
    }

    Serial.println("[Input] Initializing Input System...");

    // Initialize all devices
    for (auto& device : devices) {
        if (device) {
            device->begin();
        }
    }

    initialized = true;
    Serial.printf("[Input] System initialized with %d devices\n", devices.size());
}

void Input::loop() {
    if (!initialized) {
        Serial.println("[Input] Warning: System not initialized! Call setup() first.");
        return;
    }

    // Update all devices
    for (auto& device : devices) {
        if (device && device->isEnabled()) {
            device->update();
        }
    }
}

uint8_t Input::addButton(const ButtonConfig& config) {
    uint8_t deviceId = nextDeviceId++;

    auto button = std::make_unique<ButtonInput>(deviceId, config);

    // Set up global event handling
    button->setCallback([this](const InputEvent& event) {
        this->handleGlobalEvent(event);
    });

    devices.push_back(std::move(button));

    // Initialize immediately if system is already set up
    if (initialized) {
        devices.back()->begin();
    }

    Serial.printf("[Input] Added button %d on pin %d\n", deviceId, config.pin);
    return deviceId;
}

uint8_t Input::addRotaryEncoder(const EncoderConfig& config) {
    uint8_t deviceId = nextDeviceId++;

    auto encoder = std::make_unique<RotaryEncoderInput>(deviceId, config);

    // Set up global event handling
    encoder->setCallback([this](const InputEvent& event) {
        this->handleGlobalEvent(event);
    });

    devices.push_back(std::move(encoder));

    // Initialize immediately if system is already set up
    if (initialized) {
        devices.back()->begin();
    }

    Serial.printf("[Input] Added rotary encoder %d on pins A=%d, B=%d",
                  deviceId, config.pinA, config.pinB);
    if (config.buttonPin != 255) {
        Serial.printf(", Button=%d", config.buttonPin);
    }
    Serial.println();

    return deviceId;
}

void Input::setGlobalCallback(InputCallback callback) {
    globalCallback = callback;
}

void Input::setDeviceCallback(uint8_t deviceId, InputCallback callback) {
    InputDevice* device = getDevice(deviceId);
    if (device) {
        device->setCallback(callback);
    } else {
        Serial.printf("[Input] Warning: Device %d not found for callback assignment\n", deviceId);
    }
}

void Input::enableDevice(uint8_t deviceId) {
    InputDevice* device = getDevice(deviceId);
    if (device) {
        device->enable();
        Serial.printf("[Input] Device %d enabled\n", deviceId);
    } else {
        Serial.printf("[Input] Warning: Device %d not found for enable\n", deviceId);
    }
}

void Input::disableDevice(uint8_t deviceId) {
    InputDevice* device = getDevice(deviceId);
    if (device) {
        device->disable();
        Serial.printf("[Input] Device %d disabled\n", deviceId);
    } else {
        Serial.printf("[Input] Warning: Device %d not found for disable\n", deviceId);
    }
}

bool Input::isDeviceEnabled(uint8_t deviceId) const {
    for (const auto& device : devices) {
        if (device && device->getId() == deviceId) {
            return device->isEnabled();
        }
    }
    return false;
}

size_t Input::getDeviceCount() const {
    return devices.size();
}

void Input::clearAllDevices() {
    devices.clear();
    nextDeviceId = 0;
    Serial.println("[Input] All devices cleared");
}

void Input::printDeviceInfo() const {
    Serial.printf("[Input] System Status - Initialized: %s, Devices: %d\n",
                  initialized ? "Yes" : "No", devices.size());

    for (size_t i = 0; i < devices.size(); i++) {
        if (devices[i]) {
            Serial.printf("[Input] Device %d: ID=%d, Enabled=%s\n",
                          i, devices[i]->getId(), devices[i]->isEnabled() ? "Yes" : "No");
        }
    }
}

void Input::handleGlobalEvent(const InputEvent& event) {
    // Call global callback if set
    if (globalCallback) {
        globalCallback(event);
    }

// Debug output for events
#ifdef DEBUG_INPUT_EVENTS
    Serial.printf("[Input] Event: Device=%d, Type=%d, Value=%ld, Time=%lu\n",
                  event.deviceId, (int)event.type, event.value, event.timestamp);
#endif
}

InputDevice* Input::getDevice(uint8_t deviceId) {
    for (auto& device : devices) {
        if (device && device->getId() == deviceId) {
            return device.get();
        }
    }
    return nullptr;
}