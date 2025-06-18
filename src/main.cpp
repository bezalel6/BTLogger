// BTLogger - Bluetooth Development Log Receiver
// A tool for ESP32 developers to connect and receive log data from development devices

#include <Arduino.h>
#include "BTLoggerApp.hpp"

// Global application instance
BTLogger::BTLoggerApp app;

void setup() {
    // Initialize the application
    if (!app.initialize()) {
        Serial.println("Failed to initialize BTLogger!");
        return;
    }

    // Start the application
    app.start();
}

void loop() {
    // Update the application
    app.update();

    // Handle any input
    app.handleInput();

    // Small delay to prevent watchdog issues
    delay(1);
}