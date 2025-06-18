#pragma once

// Toggleable debug logging for BTLogger itself
// Uncomment the line below to enable BTLogger debug prints
#define DEBUG_BTLOGGER

#ifdef DEBUG_BTLOGGER
#define BTLOGGER_DEBUG(format, ...) Serial.printf("[BTLOGGER_DEBUG] " format "\n", ##__VA_ARGS__)
#else
#define BTLOGGER_DEBUG(format, ...)
#endif

/*
 * BTLoggerSender_ESPLog - Enhanced ESP_LOG integration for BTLogger
 *
 * This version hooks into the ESP-IDF logging system to automatically capture
 * all ESP_LOG* calls without requiring any code changes to existing projects.
 *
 * Features:
 * - Zero code changes needed - just include and initialize
 * - Automatically captures ESP_LOGI, ESP_LOGW, ESP_LOGE, ESP_LOGD, ESP_LOGV
 * - Preserves normal serial output while sending to BTLogger
 * - Parses log level and tag automatically
 * - Backward compatible with manual BT_LOG_* calls
 * - OVERRIDES ESP_LOG macros to enable runtime log level control
 *
 * Usage:
 * 1. Include this file instead of BTLoggerSender.hpp
 * 2. Call BTLoggerSender::begin() in setup() - that's it!
 * 3. All your existing ESP_LOG* calls will automatically be sent to BTLogger
 * 4. Log levels can now be changed at runtime, even if disabled at compile time
 *
 * Example:
 * BTLoggerSender::begin("MyProject");
 * ESP_LOGI("WIFI", "Connected to %s", ssid);  // ← Always compiled, runtime controlled!
 * ESP_LOGD("DEBUG", "Debug info: %d", value); // ← Works even if LOG_LEVEL excludes debug!
 */

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <esp_log.h>
#include <stdarg.h>
#include <stdio.h>

// Undefine existing ESP_LOG macros to override them
#ifdef ESP_LOGE
#undef ESP_LOGE
#endif
#ifdef ESP_LOGW
#undef ESP_LOGW
#endif
#ifdef ESP_LOGI
#undef ESP_LOGI
#endif
#ifdef ESP_LOGD
#undef ESP_LOGD
#endif
#ifdef ESP_LOGV
#undef ESP_LOGV
#endif

// Forward declaration of our class for use in macros
class BTLoggerSender;

// New ESP_LOG macros that enable runtime control
#define ESP_LOGE(tag, format, ...) \
    BTLoggerSender::espLogWrite(ESP_LOG_ERROR, tag, format, ##__VA_ARGS__);

#define ESP_LOGW(tag, format, ...) \
    BTLoggerSender::espLogWrite(ESP_LOG_WARN, tag, format, ##__VA_ARGS__);

#define ESP_LOGI(tag, format, ...) \
    BTLoggerSender::espLogWrite(ESP_LOG_INFO, tag, format, ##__VA_ARGS__);

#define ESP_LOGD(tag, format, ...) \
    BTLoggerSender::espLogWrite(ESP_LOG_DEBUG, tag, format, ##__VA_ARGS__);

#define ESP_LOGV(tag, format, ...) \
    BTLoggerSender::espLogWrite(ESP_LOG_VERBOSE, tag, format, ##__VA_ARGS__);

// Log levels (match both ESP_LOG and BTLogger)
enum BTLogLevel {
    BT_VERBOSE = 0,  // ESP_LOG_VERBOSE
    BT_DEBUG = 1,    // ESP_LOG_DEBUG
    BT_INFO = 2,     // ESP_LOG_INFO
    BT_WARN = 3,     // ESP_LOG_WARN
    BT_ERROR = 4     // ESP_LOG_ERROR
};

// Default UUIDs (must match BTLogger's BluetoothManager)
#define BTLOGGER_SERVICE_UUID "12345678-1234-1234-1234-123456789ABC"
#define BTLOGGER_LOG_CHAR_UUID "87654321-4321-4321-4321-CBA987654321"

// Log packet structure for communication (must match BTLogger)
struct LogPacket {
    uint32_t timestamp;
    uint8_t level;  // 0=DEBUG, 1=INFO, 2=WARN, 3=ERROR
    uint16_t length;
    char message[256];
    char tag[32];

    LogPacket() : timestamp(0), level(0), length(0) {
        message[0] = '\0';
        tag[0] = '\0';
    }
};

class BTLoggerSender {
   public:
    // Set BTLogger-specific log level (independent of global ESP_LOG_LEVEL)
    static void setBTLogLevel(BTLogLevel level) {
        BTLOGGER_DEBUG("Setting BTLogger log level from %s to %s", levelToString(_btLogLevel).c_str(), levelToString(level).c_str());
        _btLogLevel = level;
        ESP_LOGI("BTLOGGER", "BTLogger log level set to: %s", levelToString(level).c_str());
    }

    // Get current BTLogger log level
    static BTLogLevel getBTLogLevel() {
        return _btLogLevel;
    }

    // Set ESP serial log level at runtime (overrides compile-time setting)
    static void setESPLogLevel(esp_log_level_t level) {
        BTLOGGER_DEBUG("Setting ESP log level from %s to %s", espLevelToString(_espLogLevel).c_str(), espLevelToString(level).c_str());
        _espLogLevel = level;
        ESP_LOGI("BTLOGGER", "ESP serial log level set to: %s", espLevelToString(level).c_str());
    }

    // Get current ESP log level
    static esp_log_level_t getESPLogLevel() {
        return _espLogLevel;
    }

    // Core function that handles all ESP_LOG macro calls
    static void espLogWrite(esp_log_level_t esp_level, const char* tag, const char* format, ...) {
        va_list args;
        va_start(args, format);

        BTLOGGER_DEBUG("espLogWrite called - ESP level: %s, tag: %s, initialized: %s, connected: %s",
                       espLevelToString(esp_level).c_str(), tag,
                       _initialized ? "true" : "false",
                       isConnected() ? "true" : "false");

        // Check ESP log level for serial output
        if (esp_level <= _espLogLevel) {
            BTLOGGER_DEBUG("Sending to ESP serial (level %s <= %s)", espLevelToString(esp_level).c_str(), espLevelToString(_espLogLevel).c_str());
            // Call the original ESP logging function for serial output
            esp_log_writev(esp_level, tag, format, args);
        } else {
            BTLOGGER_DEBUG("Skipping ESP serial (level %s > %s)", espLevelToString(esp_level).c_str(), espLevelToString(_espLogLevel).c_str());
        }

        // Check BTLogger level and send to BTLogger
        BTLogLevel bt_level = espLevelToBTLevel(esp_level);
        if (_initialized && _logCharacteristic && bt_level >= _btLogLevel) {
            BTLOGGER_DEBUG("Sending to BTLogger (BT level %s >= %s)", levelToString(bt_level).c_str(), levelToString(_btLogLevel).c_str());
            // Format the message as LogPacket for BTLogger
            LogPacket packet;
            packet.timestamp = millis();
            packet.level = (uint8_t)bt_level;

            // Format message
            va_start(args, format);  // Reset va_list
            vsnprintf(packet.message, sizeof(packet.message), format, args);
            packet.length = strlen(packet.message);

            // Copy tag (ensure null termination)
            strncpy(packet.tag, tag, sizeof(packet.tag) - 1);
            packet.tag[sizeof(packet.tag) - 1] = '\0';

            BTLOGGER_DEBUG("BTLogger packet: timestamp=%d, level=%d, tag=%s, message=%s",
                           packet.timestamp, packet.level, packet.tag, packet.message);

            // Send as binary data
            _logCharacteristic->setValue((uint8_t*)&packet, sizeof(LogPacket));
            _logCharacteristic->notify();
            _directLogCount++;
            BTLOGGER_DEBUG("BTLogger notification sent, total count: %d", _directLogCount);
        } else {
            if (!_initialized) {
                BTLOGGER_DEBUG("Skipping BTLogger - not initialized");
            } else if (!_logCharacteristic) {
                BTLOGGER_DEBUG("Skipping BTLogger - no characteristic");
            } else {
                BTLOGGER_DEBUG("Skipping BTLogger - level check failed (BT level %s < %s)", levelToString(bt_level).c_str(), levelToString(_btLogLevel).c_str());
            }
        }

        va_end(args);
    }

    // Initialize - simplified version without ESP_LOG hooking since we override macros
    static bool begin(const String& deviceName = "ESP32_Dev", BTLogLevel btLogLevel = BT_INFO, esp_log_level_t espLogLevel = ESP_LOG_INFO) {
        if (_initialized) {
            BTLOGGER_DEBUG("begin() called but already initialized");
            return true;
        }

        Serial.println("Initializing BTLogger Sender with ESP_LOG macro override...");
        BTLOGGER_DEBUG("begin() called - device: %s, BT level: %s, ESP level: %s",
                       deviceName.c_str(), levelToString(btLogLevel).c_str(), espLevelToString(espLogLevel).c_str());

        // Set log levels
        _btLogLevel = btLogLevel;
        _espLogLevel = espLogLevel;
        BTLOGGER_DEBUG("Log levels set");

        // Initialize BLE
        BTLOGGER_DEBUG("Initializing BLE device: %s", deviceName.c_str());
        BLEDevice::init(deviceName.c_str());

        BTLOGGER_DEBUG("Creating BLE server");
        _server = BLEDevice::createServer();
        _server->setCallbacks(new ServerCallbacks());

        BTLOGGER_DEBUG("Creating BLE service: %s", BTLOGGER_SERVICE_UUID);
        BLEService* service = _server->createService(BTLOGGER_SERVICE_UUID);

        BTLOGGER_DEBUG("Creating log characteristic: %s", BTLOGGER_LOG_CHAR_UUID);
        _logCharacteristic = service->createCharacteristic(
            BTLOGGER_LOG_CHAR_UUID,
            BLECharacteristic::PROPERTY_READ |
                BLECharacteristic::PROPERTY_WRITE |
                BLECharacteristic::PROPERTY_NOTIFY);

        _logCharacteristic->addDescriptor(new BLE2902());
        BTLOGGER_DEBUG("Starting BLE service");
        service->start();

        BTLOGGER_DEBUG("Setting up BLE advertising");
        BLEAdvertising* advertising = BLEDevice::getAdvertising();
        advertising->addServiceUUID(BTLOGGER_SERVICE_UUID);
        advertising->setScanResponse(false);
        advertising->setMinPreferred(0x0);
        BLEDevice::startAdvertising();

        _initialized = true;
        BTLOGGER_DEBUG("BTLogger initialization complete");
        Serial.println("BTLogger Sender initialized with ESP_LOG macro override - Device: " + deviceName);

        // Test the integration
        ESP_LOGI("BTLOGGER", "ESP_LOG macro override active - BTLogger level: %s, ESP level: %s",
                 levelToString(_btLogLevel).c_str(), espLevelToString(_espLogLevel).c_str());

        return true;
    }

    // Manual logging (still available)
    static void log(BTLogLevel level, const String& tag, const String& message) {
        BTLOGGER_DEBUG("Manual log called - level: %s, tag: %s, message: %s", levelToString(level).c_str(), tag.c_str(), message.c_str());

        if (!_initialized || !_logCharacteristic) {
            BTLOGGER_DEBUG("Manual log skipped - initialized: %s, characteristic: %s",
                           _initialized ? "true" : "false",
                           _logCharacteristic ? "exists" : "null");
            return;
        }

        // Format as LogPacket for BTLogger
        LogPacket packet;
        packet.timestamp = millis();
        packet.level = (uint8_t)level;

        // Copy message (ensure null termination)
        strncpy(packet.message, message.c_str(), sizeof(packet.message) - 1);
        packet.message[sizeof(packet.message) - 1] = '\0';
        packet.length = strlen(packet.message);

        // Copy tag (ensure null termination)
        strncpy(packet.tag, tag.c_str(), sizeof(packet.tag) - 1);
        packet.tag[sizeof(packet.tag) - 1] = '\0';

        BTLOGGER_DEBUG("Manual log packet: timestamp=%d, level=%d, tag=%s, message=%s",
                       packet.timestamp, packet.level, packet.tag, packet.message);

        // Send as binary data
        _logCharacteristic->setValue((uint8_t*)&packet, sizeof(LogPacket));
        _logCharacteristic->notify();
        _manualLogCount++;
        BTLOGGER_DEBUG("Manual log notification sent, total count: %d", _manualLogCount);
    }

    // Convenience functions (still available)
    static void debug(const String& tag, const String& message) { log(BT_DEBUG, tag, message); }
    static void info(const String& tag, const String& message) { log(BT_INFO, tag, message); }
    static void warn(const String& tag, const String& message) { log(BT_WARN, tag, message); }
    static void error(const String& tag, const String& message) { log(BT_ERROR, tag, message); }

    // Check connection status
    static bool isConnected() {
        return _server && _server->getConnectedCount() > 0;
    }

    // Get statistics
    static uint32_t getDirectLogCount() { return _directLogCount; }
    static uint32_t getManualLogCount() { return _manualLogCount; }

    // Convenience methods for common log level scenarios
    static void setVerboseMode() {
        setBTLogLevel(BT_VERBOSE);
        setESPLogLevel(ESP_LOG_VERBOSE);
    }
    static void setDebugMode() {
        setBTLogLevel(BT_DEBUG);
        setESPLogLevel(ESP_LOG_DEBUG);
    }
    static void setInfoMode() {
        setBTLogLevel(BT_INFO);
        setESPLogLevel(ESP_LOG_INFO);
    }
    static void setWarningMode() {
        setBTLogLevel(BT_WARN);
        setESPLogLevel(ESP_LOG_WARN);
    }
    static void setErrorOnlyMode() {
        setBTLogLevel(BT_ERROR);
        setESPLogLevel(ESP_LOG_ERROR);
    }

    // Get human-readable status
    static String getStatus() {
        String status = "BTLogger Status:\n";
        status += "- Connected: " + String(isConnected() ? "Yes" : "No") + "\n";
        status += "- BTLogger Level: " + levelToString(_btLogLevel) + "\n";
        status += "- ESP Serial Level: " + espLevelToString(_espLogLevel) + "\n";
        status += "- Direct ESP_LOG messages: " + String(_directLogCount) + "\n";
        status += "- Manual logs sent: " + String(_manualLogCount);
        return status;
    }

   private:
    static bool _initialized;
    static BLEServer* _server;
    static BLECharacteristic* _logCharacteristic;
    static uint32_t _directLogCount;
    static uint32_t _manualLogCount;
    static BTLogLevel _btLogLevel;
    static esp_log_level_t _espLogLevel;

    // Convert ESP log level to BTLogger level
    static BTLogLevel espLevelToBTLevel(esp_log_level_t esp_level) {
        switch (esp_level) {
            case ESP_LOG_NONE:
                return BT_ERROR;  // Map to highest BTLogger level
            case ESP_LOG_ERROR:
                return BT_ERROR;
            case ESP_LOG_WARN:
                return BT_WARN;
            case ESP_LOG_INFO:
                return BT_INFO;
            case ESP_LOG_DEBUG:
                return BT_DEBUG;
            case ESP_LOG_VERBOSE:
                return BT_VERBOSE;
            default:
                return BT_INFO;
        }
    }

    static String levelToString(BTLogLevel level) {
        switch (level) {
            case BT_VERBOSE:
                return "VERB";
            case BT_DEBUG:
                return "DEBUG";
            case BT_INFO:
                return "INFO";
            case BT_WARN:
                return "WARN";
            case BT_ERROR:
                return "ERROR";
            default:
                return "UNKN";
        }
    }

    static String espLevelToString(esp_log_level_t level) {
        switch (level) {
            case ESP_LOG_NONE:
                return "NONE";
            case ESP_LOG_ERROR:
                return "ERROR";
            case ESP_LOG_WARN:
                return "WARN";
            case ESP_LOG_INFO:
                return "INFO";
            case ESP_LOG_DEBUG:
                return "DEBUG";
            case ESP_LOG_VERBOSE:
                return "VERBOSE";
            default:
                return "UNKNOWN";
        }
    }

    // BLE Server callbacks
    class ServerCallbacks : public BLEServerCallbacks {
        void onConnect(BLEServer* server) override {
            BTLOGGER_DEBUG("BLE client connected - server has %d connections", server->getConnectedCount());
            Serial.println("BTLogger connected!");
            ESP_LOGI("BTLOGGER", "BTLogger device connected via BLE");
        }

        void onDisconnect(BLEServer* server) override {
            BTLOGGER_DEBUG("BLE client disconnected - server has %d connections remaining", server->getConnectedCount());
            Serial.println("BTLogger disconnected - Restarting advertising...");
            ESP_LOGW("BTLOGGER", "BTLogger device disconnected - restarting advertising");
            BTLOGGER_DEBUG("Restarting BLE advertising");
            BLEDevice::startAdvertising();
        }
    };
};

// Static member initialization
bool BTLoggerSender::_initialized = false;
BLEServer* BTLoggerSender::_server = nullptr;
BLECharacteristic* BTLoggerSender::_logCharacteristic = nullptr;
uint32_t BTLoggerSender::_directLogCount = 0;
uint32_t BTLoggerSender::_manualLogCount = 0;
BTLogLevel BTLoggerSender::_btLogLevel = BT_INFO;
esp_log_level_t BTLoggerSender::_espLogLevel = ESP_LOG_INFO;

// Convenience macros (still available for manual use)
#define BT_LOG_DEBUG(tag, msg) BTLoggerSender::debug(tag, msg)
#define BT_LOG_INFO(tag, msg) BTLoggerSender::info(tag, msg)
#define BT_LOG_WARN(tag, msg) BTLoggerSender::warn(tag, msg)
#define BT_LOG_ERROR(tag, msg) BTLoggerSender::error(tag, msg)