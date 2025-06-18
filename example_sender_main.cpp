/*
 * Example ESP32 Development Project using BTLoggerSender
 *
 * This example shows how to integrate BTLogger logging into your ESP32 development project.
 * The BTLogger device will connect to this ESP32 and receive all log messages in real-time.
 *
 * Prerequisites:
 * 1. BTLogger device running and scanning for devices
 * 2. ESP32-BLE-Arduino library installed
 * 3. BTLoggerSender.hpp included in your project
 */

#include <Arduino.h>
#include "BTLoggerSender.hpp"

// Simulated sensor values
float temperature = 25.0;
float humidity = 60.0;
int errorCount = 0;

void setup() {
    Serial.begin(115200);
    Serial.println("Starting ESP32 Development Project with BTLogger integration");

    // Initialize BTLogger sender
    BTLoggerSender::begin("MyDeviceProject_v1.2");

    // Log system startup
    BT_LOG_INFO("SYSTEM", "Device started successfully");
    BT_LOG_INFO("SYSTEM", "Firmware version: 1.2.0");
    BT_LOG_INFO("SYSTEM", "Free heap: " + String(ESP.getFreeHeap()) + " bytes");

    // Simulate some initialization logs
    delay(1000);
    BT_LOG_INFO("WIFI", "Attempting to connect to WiFi...");
    delay(2000);
    BT_LOG_WARN("WIFI", "WiFi connection timeout, continuing without WiFi");

    BT_LOG_INFO("SENSORS", "Initializing temperature sensor...");
    delay(500);
    BT_LOG_INFO("SENSORS", "Temperature sensor initialized successfully");

    BT_LOG_INFO("SENSORS", "Initializing humidity sensor...");
    delay(500);
    BT_LOG_INFO("SENSORS", "Humidity sensor initialized successfully");

    BT_LOG_INFO("SYSTEM", "Setup complete - entering main loop");
}

void loop() {
    static unsigned long lastSensorRead = 0;
    static unsigned long lastStatusUpdate = 0;
    static unsigned long lastDebugMessage = 0;

    unsigned long currentTime = millis();

    // Read sensors every 5 seconds
    if (currentTime - lastSensorRead > 5000) {
        readSensors();
        lastSensorRead = currentTime;
    }

    // Send status update every 30 seconds
    if (currentTime - lastStatusUpdate > 30000) {
        sendStatusUpdate();
        lastStatusUpdate = currentTime;
    }

    // Send debug messages occasionally
    if (currentTime - lastDebugMessage > 15000) {
        sendDebugInfo();
        lastDebugMessage = currentTime;
    }

    // Simulate some random events
    if (random(1000) < 2) {  // 0.2% chance per loop
        simulateRandomEvent();
    }

    delay(100);
}

void readSensors() {
    BT_LOG_DEBUG("SENSORS", "Reading sensor values...");

    // Simulate temperature reading with some variation
    temperature += random(-100, 100) / 100.0;
    temperature = constrain(temperature, 20.0, 30.0);

    // Simulate humidity reading
    humidity += random(-200, 200) / 100.0;
    humidity = constrain(humidity, 40.0, 80.0);

    // Log sensor readings
    String tempMsg = "Temperature: " + String(temperature, 1) + "°C";
    String humMsg = "Humidity: " + String(humidity, 1) + "%";

    BT_LOG_INFO("TEMP_SENSOR", tempMsg);
    BT_LOG_INFO("HUM_SENSOR", humMsg);

    // Check for sensor errors (simulated)
    if (temperature > 28.0) {
        BT_LOG_WARN("TEMP_SENSOR", "Temperature above normal range");
        errorCount++;
    }

    if (humidity < 45.0) {
        BT_LOG_WARN("HUM_SENSOR", "Humidity below normal range");
        errorCount++;
    }

    // Simulate occasional sensor communication errors
    if (random(100) < 5) {  // 5% chance
        BT_LOG_ERROR("SENSORS", "Communication timeout with sensor module");
        errorCount++;
    }
}

void sendStatusUpdate() {
    BT_LOG_INFO("STATUS", "=== System Status Update ===");
    BT_LOG_INFO("STATUS", "Uptime: " + String(millis() / 1000) + " seconds");
    BT_LOG_INFO("STATUS", "Free heap: " + String(ESP.getFreeHeap()) + " bytes");
    BT_LOG_INFO("STATUS", "Total errors: " + String(errorCount));
    BT_LOG_INFO("STATUS", "BTLogger connected: " + String(BTLoggerSender::isConnected() ? "Yes" : "No"));

    // Reset error count periodically
    if (errorCount > 10) {
        BT_LOG_WARN("STATUS", "Error count reset after reaching threshold");
        errorCount = 0;
    }
}

void sendDebugInfo() {
    BT_LOG_DEBUG("DEBUG", "Debug message #" + String(millis() / 1000));
    BT_LOG_DEBUG("MEMORY", "Stack high water mark: " + String(uxTaskGetStackHighWaterMark(NULL)));
    BT_LOG_DEBUG("TIMING", "Loop execution time: ~100ms");
}

void simulateRandomEvent() {
    int eventType = random(4);

    switch (eventType) {
        case 0:
            BT_LOG_INFO("EVENT", "User button pressed");
            break;

        case 1:
            BT_LOG_WARN("EVENT", "Low battery warning");
            break;

        case 2:
            BT_LOG_ERROR("EVENT", "Unexpected sensor disconnect");
            errorCount++;
            break;

        case 3:
            BT_LOG_INFO("EVENT", "Firmware update check completed");
            break;
    }
}

/*
 * Example PlatformIO platformio.ini configuration for this project:
 *
 * [env:esp32dev]
 * platform = espressif32
 * board = esp32dev
 * framework = arduino
 * monitor_speed = 115200
 * lib_deps =
 *     ESP32-BLE-Arduino
 *
 * Usage Instructions:
 * 1. Copy BTLoggerSender.hpp to your project folder
 * 2. Replace your main.cpp with this example (or integrate the relevant parts)
 * 3. Build and upload to your ESP32
 * 4. Power on your BTLogger device
 * 5. Watch the logs appear in real-time on the BTLogger display
 * 6. Save/browse logs using the BTLogger SD card functionality
 */