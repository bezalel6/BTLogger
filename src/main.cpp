// BTLogger - Bluetooth Development Log Receiver
// A tool for ESP32 developers to connect and receive log data from development devices

#include <Arduino.h>
#include "BTLoggerApp.hpp"

// Global application instance
BTLoggerApp* app = nullptr;

void setup(void) {
    // Create and initialize the BTLogger application
    app = new BTLoggerApp();

    if (app->initialize()) {
        app->start();
        Serial.println("BTLogger application started successfully!");
    } else {
        Serial.println("FATAL: Failed to initialize BTLogger application");
        while (1) {
            delay(1000);
        }
    }
}

void loop(void) {
    if (app) {
        app->update();
    }

    // Small delay to prevent watchdog issues
    delay(1);
}