#pragma once

/*
 * BTLoggerSender - Simple library for ESP32 development boards to send logs to BTLogger
 *
 * Usage:
 * 1. Include this file in your ESP32 project
 * 2. Call BTLoggerSender::begin() in setup()
 * 3. Use BTLoggerSender::log() to send log messages
 *
 * Example:
 * BTLoggerSender::begin("My_ESP32_Device");
 * BTLoggerSender::log(INFO, "MAIN", "System started successfully");
 * BTLoggerSender::log(ERROR, "SENSOR", "Failed to read temperature");
 */

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Log levels (match BTLogger's LogPacket structure)
enum BTLogLevel {
    BT_DEBUG = 0,
    BT_INFO = 1,
    BT_WARN = 2,
    BT_ERROR = 3
};

// Default UUIDs (must match BTLogger's BluetoothManager)
#define BTLOGGER_SERVICE_UUID "12345678-1234-1234-1234-123456789abc"
#define BTLOGGER_LOG_CHAR_UUID "87654321-4321-4321-4321-cba987654321"

class BTLoggerSender {
   public:
    // Initialize the BLE service for sending logs
    static bool begin(const String& deviceName = "ESP32_Dev") {
        if (_initialized) return true;

        Serial.println("Initializing BTLogger Sender...");

        // Initialize BLE
        BLEDevice::init(deviceName.c_str());

        // Create BLE Server
        _server = BLEDevice::createServer();
        _server->setCallbacks(new ServerCallbacks());

        // Create BLE Service
        BLEService* service = _server->createService(BTLOGGER_SERVICE_UUID);

        // Create BLE Characteristic
        _logCharacteristic = service->createCharacteristic(
            BTLOGGER_LOG_CHAR_UUID,
            BLECharacteristic::PROPERTY_READ |
                BLECharacteristic::PROPERTY_WRITE |
                BLECharacteristic::PROPERTY_NOTIFY);

        _logCharacteristic->addDescriptor(new BLE2902());

        // Start service
        service->start();

        // Start advertising
        BLEAdvertising* advertising = BLEDevice::getAdvertising();
        advertising->addServiceUUID(BTLOGGER_SERVICE_UUID);
        advertising->setScanResponse(false);
        advertising->setMinPreferred(0x0);  // Set value to 0x00 to not advertise this parameter
        BLEDevice::startAdvertising();

        _initialized = true;
        Serial.println("BTLogger Sender initialized - Device discoverable as: " + deviceName);

        // Send startup log
        log(BT_INFO, "BTLOGGER", "BTLogger Sender initialized");

        return true;
    }

    // Send a log message
    static void log(BTLogLevel level, const String& tag, const String& message) {
        if (!_initialized || !_logCharacteristic) return;

        // Create log packet (simple text format for now)
        String logEntry = "[" + String(millis()) + "] [" + levelToString(level) + "] [" + tag + "] " + message;

        // Send via BLE
        _logCharacteristic->setValue(logEntry.c_str());
        _logCharacteristic->notify();

        // Also print to serial for local debugging
        Serial.println(logEntry);
    }

    // Convenience macros
    static void debug(const String& tag, const String& message) { log(BT_DEBUG, tag, message); }
    static void info(const String& tag, const String& message) { log(BT_INFO, tag, message); }
    static void warn(const String& tag, const String& message) { log(BT_WARN, tag, message); }
    static void error(const String& tag, const String& message) { log(BT_ERROR, tag, message); }

    // Check if a BTLogger is connected
    static bool isConnected() {
        return _server && _server->getConnectedCount() > 0;
    }

   private:
    static bool _initialized;
    static BLEServer* _server;
    static BLECharacteristic* _logCharacteristic;

    static String levelToString(BTLogLevel level) {
        switch (level) {
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

    // BLE Server callbacks
    class ServerCallbacks : public BLEServerCallbacks {
        void onConnect(BLEServer* server) override {
            Serial.println("BTLogger connected!");
        }

        void onDisconnect(BLEServer* server) override {
            Serial.println("BTLogger disconnected - Restarting advertising...");
            BLEDevice::startAdvertising();
        }
    };
};

// Static member initialization
bool BTLoggerSender::_initialized = false;
BLEServer* BTLoggerSender::_server = nullptr;
BLECharacteristic* BTLoggerSender::_logCharacteristic = nullptr;

// Convenience macros for even easier usage
#define BT_LOG_DEBUG(tag, msg) BTLoggerSender::debug(tag, msg)
#define BT_LOG_INFO(tag, msg) BTLoggerSender::info(tag, msg)
#define BT_LOG_WARN(tag, msg) BTLoggerSender::warn(tag, msg)
#define BT_LOG_ERROR(tag, msg) BTLoggerSender::error(tag, msg)