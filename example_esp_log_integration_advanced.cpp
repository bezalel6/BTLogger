/*
 * Advanced ESP_LOG Integration Example with Independent Log Levels
 *
 * This example demonstrates how to have different log levels for:
 * 1. Local serial output (controlled by ESP_LOG_LEVEL in build settings)
 * 2. BTLogger remote logging (controlled by BTLoggerSender log level)
 *
 * Scenario:
 * - Production device only logs ERRORs locally (for performance)
 * - BTLogger captures INFO+ for remote debugging
 * - No code changes needed - just configuration!
 */

#include <Arduino.h>
#include "BTLoggerSender_ESPLog.hpp"

// In your platformio.ini, you might have:
// build_flags = -DCORE_DEBUG_LEVEL=1  ; Only ERROR locally
// But BTLogger can still capture INFO+ for remote debugging!

void setup() {
    Serial.begin(115200);

    // Initialize BTLogger with independent log level
    // Local ESP_LOG_LEVEL might be ERROR, but BTLogger gets INFO+
    BTLoggerSender::begin("ProductionDevice_v1.0", true, BT_INFO);

    // Alternative: Use convenience methods
    // BTLoggerSender::begin("ProductionDevice_v1.0");
    // BTLoggerSender::setInfoMode();  // INFO, WARN, ERROR to BTLogger

    ESP_LOGI("SYSTEM", "Device started - this will be sent to BTLogger even if local ESP_LOG_LEVEL=ERROR");
    ESP_LOGW("SYSTEM", "This warning goes to BTLogger too");
    ESP_LOGE("SYSTEM", "Errors always go everywhere");
    ESP_LOGD("SYSTEM", "Debug messages are filtered out (BTLogger level = INFO)");

    Serial.println("\n" + BTLoggerSender::getStatus());
}

void loop() {
    static unsigned long lastDemo = 0;
    static int demoStep = 0;

    if (millis() - lastDemo > 10000) {  // Every 10 seconds

        switch (demoStep) {
            case 0:
                demonstrateScenario1();
                break;
            case 1:
                demonstrateScenario2();
                break;
            case 2:
                demonstrateScenario3();
                break;
            case 3:
                showStatistics();
                demoStep = -1;  // Reset
                break;
        }

        demoStep++;
        lastDemo = millis();
    }

    delay(100);
}

void demonstrateScenario1() {
    Serial.println("\n=== Scenario 1: Production Debugging ===");
    Serial.println("Local ESP_LOG_LEVEL=ERROR, BTLogger=INFO");

    // Set BTLogger to INFO mode (INFO, WARN, ERROR)
    BTLoggerSender::setInfoMode();

    ESP_LOGD("SENSOR", "Reading temperature sensor...");     // Filtered locally & BTLogger
    ESP_LOGI("SENSOR", "Temperature: 23.5°C");               // Filtered locally, sent to BTLogger
    ESP_LOGW("SENSOR", "Temperature sensor slow response");  // Sent to BTLogger
    ESP_LOGE("SENSOR", "Temperature sensor failure!");       // Sent everywhere

    Serial.println("-> INFO/WARN/ERROR sent to BTLogger for remote debugging");
}

void demonstrateScenario2() {
    Serial.println("\n=== Scenario 2: Critical Systems ===");
    Serial.println("Local ESP_LOG_LEVEL=ERROR, BTLogger=WARN");

    // Set BTLogger to only capture warnings and errors
    BTLoggerSender::setWarningMode();

    ESP_LOGI("MOTOR", "Motor started successfully");     // Filtered locally & BTLogger
    ESP_LOGW("MOTOR", "Motor temperature rising");       // Sent to BTLogger
    ESP_LOGE("MOTOR", "Motor overheating - shutdown!");  // Sent everywhere

    Serial.println("-> Only WARN/ERROR sent to BTLogger");
}

void demonstrateScenario3() {
    Serial.println("\n=== Scenario 3: Development Mode ===");
    Serial.println("Local ESP_LOG_LEVEL=INFO, BTLogger=DEBUG");

    // Set BTLogger to capture everything for development
    BTLoggerSender::setDebugMode();

    ESP_LOGD("WIFI", "Scanning for networks...");  // Sent to BTLogger only
    ESP_LOGI("WIFI", "Found 5 networks");          // Sent everywhere
    ESP_LOGW("WIFI", "Weak signal strength");      // Sent everywhere
    ESP_LOGE("WIFI", "Connection failed");         // Sent everywhere

    Serial.println("-> ALL messages sent to BTLogger for detailed debugging");
}

void showStatistics() {
    Serial.println("\n=== BTLogger Statistics ===");
    Serial.println(BTLoggerSender::getStatus());

    // You can also change log level at runtime
    Serial.println("\n=== Runtime Log Level Changes ===");
    Serial.println("Switching BTLogger to ERROR-only mode...");
    BTLoggerSender::setErrorOnlyMode();

    ESP_LOGI("TEST", "This INFO won't reach BTLogger now");
    ESP_LOGE("TEST", "But this ERROR will");

    // Switch back
    BTLoggerSender::setInfoMode();
    ESP_LOGI("TEST", "INFO messages work again!");
}

/*
 * Example platformio.ini for this scenario:
 *
 * [env:esp32dev]
 * platform = espressif32
 * board = esp32dev
 * framework = arduino
 * monitor_speed = 115200
 *
 * ; Local serial logging: ERROR only (for performance)
 * build_flags =
 *     -DCORE_DEBUG_LEVEL=1
 *     -DLOG_LOCAL_LEVEL=ESP_LOG_ERROR
 *
 * lib_deps =
 *     ESP32-BLE-Arduino
 *
 * Usage Benefits:
 *
 * 1. PERFORMANCE: Local device only processes/stores critical errors
 * 2. REMOTE DEBUGGING: BTLogger gets detailed logs for troubleshooting
 * 3. ZERO CODE CHANGES: Just configuration differences
 * 4. RUNTIME CONTROL: Change BTLogger level without recompiling
 * 5. INDEPENDENT: BTLogger level completely separate from ESP_LOG_LEVEL
 *
 * Perfect for:
 * - Production devices that need minimal local logging
 * - Remote debugging without impacting device performance
 * - Field troubleshooting with detailed logs
 * - Development vs production configurations
 */