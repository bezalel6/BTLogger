/*
 * ESP_LOG Integration Example for BTLogger
 *
 * This example demonstrates how BTLogger seamlessly integrates with existing
 * ESP32 projects that use the standard ESP_LOG* macros. No code changes needed!
 *
 * Key Benefits:
 * 1. Zero code changes to existing projects
 * 2. All ESP_LOGI, ESP_LOGW, ESP_LOGE, ESP_LOGD calls automatically sent to BTLogger
 * 3. Normal serial output preserved
 * 4. Works with any existing ESP32 codebase
 *
 * Usage:
 * 1. Replace BTLoggerSender.hpp with BTLoggerSender_ESPLog.hpp
 * 2. Add one line: BTLoggerSender::begin("MyProject")
 * 3. All existing ESP_LOG* calls automatically work with BTLogger!
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "BTLoggerSender_ESPLog.hpp"  // ← Enhanced version with ESP_LOG hook

// This is a typical ESP32 project using standard ESP_LOG macros
// No changes needed - BTLogger will automatically capture all logs!

const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";
const char* serverURL = "http://api.example.com/data";

// Sensor simulation
float temperature = 0.0;
float humidity = 0.0;
int sensorErrors = 0;

void setup() {
    Serial.begin(115200);

    // ===== ONLY ADDITION NEEDED FOR BTLOGGER =====
    BTLoggerSender::begin("WeatherStation_v2.1");  // ← This is the ONLY line you need to add!
    // ==============================================

    // All the code below is standard ESP32 code using ESP_LOG
    // Every ESP_LOG* call will automatically be sent to BTLogger!

    ESP_LOGI("SYSTEM", "Weather Station starting up...");
    ESP_LOGI("SYSTEM", "Firmware version: 2.1.0");
    ESP_LOGI("SYSTEM", "Free heap: %d bytes", ESP.getFreeHeap());

    // Initialize hardware
    initializeSensors();

    // Connect to WiFi (using standard ESP_LOG calls)
    connectWiFi();

    ESP_LOGI("SYSTEM", "Setup complete - entering main loop");
}

void loop() {
    static unsigned long lastSensorRead = 0;
    static unsigned long lastDataUpload = 0;

    unsigned long currentTime = millis();

    // Read sensors every 10 seconds
    if (currentTime - lastSensorRead > 10000) {
        readSensors();
        lastSensorRead = currentTime;
    }

    // Upload data every 60 seconds
    if (currentTime - lastDataUpload > 60000) {
        uploadSensorData();
        lastDataUpload = currentTime;
    }

    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
        ESP_LOGW("WIFI", "WiFi connection lost - attempting reconnection");
        connectWiFi();
    }

    delay(1000);
}

void initializeSensors() {
    ESP_LOGI("SENSORS", "Initializing sensor hardware...");

    // Simulate sensor initialization
    delay(500);

    ESP_LOGI("DHT22", "Temperature sensor initialized");
    ESP_LOGI("DHT22", "Humidity sensor initialized");

    // Simulate occasional initialization issues
    if (random(10) < 2) {  // 20% chance
        ESP_LOGW("SENSORS", "Sensor calibration took longer than expected");
    }

    ESP_LOGI("SENSORS", "All sensors ready");
}

void connectWiFi() {
    ESP_LOGI("WIFI", "Connecting to WiFi network: %s", ssid);

    WiFi.begin(ssid, password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        attempts++;
        ESP_LOGD("WIFI", "Connection attempt %d/20", attempts);

        if (attempts == 10) {
            ESP_LOGW("WIFI", "Connection taking longer than expected...");
        }
    }

    if (WiFi.status() == WL_CONNECTED) {
        ESP_LOGI("WIFI", "Connected successfully!");
        ESP_LOGI("WIFI", "IP address: %s", WiFi.localIP().toString().c_str());
        ESP_LOGI("WIFI", "Signal strength: %d dBm", WiFi.RSSI());
    } else {
        ESP_LOGE("WIFI", "Failed to connect after %d attempts", attempts);
        ESP_LOGE("WIFI", "Check SSID and password");
    }
}

void readSensors() {
    ESP_LOGD("SENSORS", "Reading sensor values...");

    // Simulate sensor readings with some variation and occasional errors
    if (random(100) < 5) {  // 5% chance of sensor error
        ESP_LOGE("DHT22", "Failed to read from sensor - communication timeout");
        sensorErrors++;
        return;
    }

    // Simulate realistic sensor data
    temperature = 20.0 + random(-50, 150) / 10.0;  // 15.0 to 35.0°C
    humidity = 40.0 + random(0, 400) / 10.0;       // 40.0 to 80.0%

    ESP_LOGI("DHT22", "Temperature: %.1f°C", temperature);
    ESP_LOGI("DHT22", "Humidity: %.1f%%", humidity);

    // Check for warning conditions
    if (temperature > 30.0) {
        ESP_LOGW("TEMP", "High temperature detected: %.1f°C", temperature);
    }

    if (temperature < 0.0) {
        ESP_LOGW("TEMP", "Sub-zero temperature detected: %.1f°C", temperature);
    }

    if (humidity > 75.0) {
        ESP_LOGW("HUMIDITY", "High humidity detected: %.1f%%", humidity);
    }

    if (humidity < 20.0) {
        ESP_LOGE("HUMIDITY", "Critically low humidity: %.1f%%", humidity);
    }

    // Log sensor health
    if (sensorErrors > 0) {
        ESP_LOGW("SENSORS", "Total sensor errors: %d", sensorErrors);
    }
}

void uploadSensorData() {
    if (WiFi.status() != WL_CONNECTED) {
        ESP_LOGE("UPLOAD", "Cannot upload - WiFi not connected");
        return;
    }

    ESP_LOGI("UPLOAD", "Uploading sensor data to server...");

    HTTPClient http;
    http.begin(serverURL);
    http.addHeader("Content-Type", "application/json");

    // Create JSON payload
    StaticJsonDocument<200> doc;
    doc["device_id"] = "weather_station_01";
    doc["timestamp"] = millis();
    doc["temperature"] = temperature;
    doc["humidity"] = humidity;
    doc["errors"] = sensorErrors;

    String jsonString;
    serializeJson(doc, jsonString);

    ESP_LOGD("UPLOAD", "Payload: %s", jsonString.c_str());

    // Send POST request
    int httpResponseCode = http.POST(jsonString);

    if (httpResponseCode > 0) {
        String response = http.getString();
        ESP_LOGI("UPLOAD", "Server response code: %d", httpResponseCode);

        if (httpResponseCode == 200) {
            ESP_LOGI("UPLOAD", "Data uploaded successfully");
        } else {
            ESP_LOGW("UPLOAD", "Server returned non-200 status: %d", httpResponseCode);
        }
    } else {
        ESP_LOGE("UPLOAD", "HTTP request failed with error: %s", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
}

/*
 * Example platformio.ini for this project:
 *
 * [env:esp32dev]
 * platform = espressif32
 * board = esp32dev
 * framework = arduino
 * monitor_speed = 115200
 * lib_deps =
 *     ESP32-BLE-Arduino
 *     bblanchon/ArduinoJson@^6.19.4
 *
 * Key Points:
 *
 * 1. ZERO CODE CHANGES needed to existing ESP_LOG calls
 * 2. Just include BTLoggerSender_ESPLog.hpp instead of standard header
 * 3. Add BTLoggerSender::begin() - that's it!
 * 4. All ESP_LOGI, ESP_LOGW, ESP_LOGE, ESP_LOGD automatically sent to BTLogger
 * 5. Normal serial output is preserved unchanged
 * 6. Works with any existing ESP32 project using ESP_LOG
 *
 * Before BTLogger Integration:
 * ESP_LOGI("WIFI", "Connected to %s", ssid);  // Only goes to Serial
 *
 * After BTLogger Integration:
 * ESP_LOGI("WIFI", "Connected to %s", ssid);  // Goes to Serial AND BTLogger!
 *
 * The same exact line of code now sends logs wirelessly to your BTLogger device!
 */