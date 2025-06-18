# BTLogger - Bluetooth Development Log Receiver

A professional Bluetooth logging suite for ESP32 smart display boards that connects to development ESP32 devices and provides real-time log display, storage, and management.

## 🎯 Overview

BTLogger transforms your ESP32 smart display into a powerful debugging tool that wirelessly receives and displays log messages from other ESP32 development boards. Perfect for debugging IoT projects, monitoring sensor data, and analyzing system behavior without physical serial connections.

## ✨ Features

### Core Functionality
- **🔄 Real-time Bluetooth Log Reception** - Connect to development ESP32s and receive logs instantly
- **📱 Professional LVGL GUI** - Clean, responsive touch interface with scrollable log viewer
- **💾 SD Card Logging** - Automatic session saving with organized file structure
- **🔍 Log Management** - Browse, load, and delete previous log sessions
- **🚥 Visual Status Indicators** - RGB LEDs show power, connection, and SD card status
- **⚡ Hardware Input Support** - Buttons and rotary encoder for navigation

### Technical Features
- **📡 BLE Central Mode** - Connects to multiple development devices
- **📝 Structured Log Format** - Supports DEBUG, INFO, WARN, ERROR levels with timestamps
- **🗂️ Organized Storage** - Automatic session management with device-specific folders
- **🎨 Color-coded Display** - Different colors for log levels and connection status
- **⚙️ Touch + Hardware Controls** - Both touchscreen and physical button support

### Integration Options
- **🔧 ESP_LOG Integration** - Zero-code-change integration with existing ESP32 projects
- **📚 Manual API** - Custom logging calls for new projects
- **🔄 Hybrid Mode** - Both ESP_LOG and manual calls in same project

## 🛠️ Hardware Requirements

### BTLogger Device (This Project)
- **ESP32 Smart Display Board**: ESP32_2432S028R, ESP32_3248S035R, or ESP32_3248S035C
- **Display**: 240x320 ILI9341 TFT with XPT2046 touch controller
- **Storage**: MicroSD card (any size)
- **Inputs**: Hardware buttons/rotary encoder (optional)
- **Indicators**: RGB LEDs for status

### Development Devices (Your Projects)
- Any ESP32 board with Bluetooth capability
- ESP32-BLE-Arduino library support

## 🚀 Quick Start

### 1. Setup BTLogger Device

#### Hardware Setup
```
SD Card Pins:
- SCK:  GPIO 18
- MISO: GPIO 19  
- MOSI: GPIO 23
- CS:   GPIO 5

Status LEDs:
- Red (Power):     GPIO 17
- Green (BT):      GPIO 4  
- Blue (SD):       GPIO 16

Optional Hardware Controls:
- Encoder A:       GPIO 27
- Encoder B:       GPIO 22
- Encoder Button:  GPIO 5
```

#### Software Setup
1. **Clone and Build**:
   ```bash
   git clone <this-repo>
   cd BTLogger
   pio run -t upload
   ```

2. **Insert SD Card** - BTLogger will automatically create the folder structure:
   ```
   /logs/
     ├─ sessions/     # Log session files
     ├─ config/       # Configuration files
     └─ ...
   ```

3. **Power On** - The device will:
   - Initialize LVGL display system
   - Start Bluetooth scanning
   - Show "BTLogger Ready" screen

### 2. Setup Development Device

#### Option A: ESP_LOG Integration (Recommended - Zero Code Changes!)

For existing ESP32 projects using `ESP_LOGI`, `ESP_LOGW`, `ESP_LOGE`, etc:

1. **Copy `BTLoggerSender_ESPLog.hpp`** to your project folder

2. **Add include and one line to setup()**:
   ```cpp
   #include "BTLoggerSender_ESPLog.hpp"
   
   void setup() {
       Serial.begin(115200);
       
       // ← ONLY line you need to add!
       BTLoggerSender::begin("MyProject_v1.0");
       
       // ALL existing ESP_LOG calls now automatically sent to BTLogger!
       ESP_LOGI("WIFI", "Connecting to network...");    // ← No changes needed!
       ESP_LOGW("SENSOR", "Temperature high: %.1f", temp); // ← Works automatically!
       ESP_LOGE("SYSTEM", "Memory allocation failed");     // ← Also captured!
   }
   ```

3. **Add dependency** to `platformio.ini`:
   ```ini
   lib_deps = 
       ESP32-BLE-Arduino
   ```

**That's it!** All your existing `ESP_LOG*` calls will automatically appear on BTLogger with zero code changes.

#### Option B: Manual API Integration

For new projects or when you want explicit control:

1. **Copy `BTLoggerSender.hpp`** to your project folder

2. **Update your main code**:
   ```cpp
   #include "BTLoggerSender.hpp"
   
   void setup() {
       Serial.begin(115200);
       
       // Initialize BTLogger sender
       BTLoggerSender::begin("MyProject_v1.0");
       
       // Send logs using BTLogger API
       BT_LOG_INFO("SYSTEM", "Device started successfully");
       BT_LOG_WARN("SENSOR", "Temperature sensor not responding");
       BT_LOG_ERROR("WIFI", "Connection failed");
   }
   
   void loop() {
       // Your code here...
       
       // Send periodic status
       BT_LOG_DEBUG("LOOP", "Main loop iteration complete");
       delay(1000);
   }
   ```

### 3. Connect and Use

1. **Power on BTLogger** - it will start scanning automatically
2. **Upload and run** your development project with BTLoggerSender
3. **Watch connection** - BTLogger will auto-discover and connect
4. **View logs** - Real-time logs appear on the scrollable display
5. **Save session** - Logs are automatically saved to SD card

## 📖 User Interface Guide

### Main Log Viewer Screen
- **Title Bar**: Shows "BTLogger - Log Viewer"
- **Connection Status**: Shows connected device name (green) or "No device connected" (red)
- **Log Display**: Scrollable text area with real-time log entries
- **Controls**: 
  - `Auto-Scroll: ON/OFF` - Toggle automatic scrolling
  - `Clear` - Clear current display
- **Navigation Hint**: Shows hardware button instructions

### Log Entry Format
```
[timestamp] [LEVEL] [TAG] message {device_name}
```
Example:
```
[15234] [INFO] [SENSOR] Temperature: 25.3°C {MyProject_v1.0}
[15890] [WARN] [WIFI] Connection timeout {MyProject_v1.0}  
[16123] [ERROR] [SYSTEM] Out of memory {MyProject_v1.0}
```

### LED Status Indicators
- **🔴 Red LED**: Power/System status (on when running)
- **🟢 Green LED**: Bluetooth connection (on when device connected)
- **🔵 Blue LED**: SD Card activity (blinks during logging, solid when card present)

## 🗂️ File Management

### Automatic Session Management
BTLogger automatically creates log sessions:
- **Session Start**: When a device connects
- **Session End**: When device disconnects or manually stopped
- **File Naming**: `TIMESTAMP_devicename.log`
- **Auto-rotation**: New session when file size exceeds 10MB

### Log File Structure
```
# BTLogger Session Log
# Session: 15-14-32-45_MyProject_v1.0.log
# Device: MyProject_v1.0
# Started: 15234567
# Format: [TIMESTAMP] [LEVEL] [TAG] MESSAGE
# =====================================

[15234] [INFO] [SYSTEM] Device started successfully {MyProject_v1.0}
[15890] [WARN] [WIFI] Connection timeout {MyProject_v1.0}
...
# Session ended: 18234567
# Total size: 2048 bytes
```

## 🔧 Development Integration

### ESP_LOG Integration (Zero Code Changes!)

The enhanced ESP_LOG integration hooks into the ESP-IDF logging system:

#### How It Works
```cpp
#include "BTLoggerSender_ESPLog.hpp"

void setup() {
    BTLoggerSender::begin("MyDevice");  // ← Hooks into ESP_LOG system
    
    // ALL of these existing calls automatically sent to BTLogger:
    ESP_LOGI("MAIN", "System started");           // ← Works!
    ESP_LOGW("WIFI", "Weak signal: %d", rssi);   // ← Works!
    ESP_LOGE("SENSOR", "Read failed");           // ← Works!
    ESP_LOGD("DEBUG", "Debug info: %s", data);   // ← Works!
}
```

#### Before/After Comparison
```cpp
// Before BTLogger (normal ESP32 project):
ESP_LOGI("WIFI", "Connected to %s", ssid);  // ← Only goes to Serial

// After BTLogger (same exact line!):
ESP_LOGI("WIFI", "Connected to %s", ssid);  // ← Goes to Serial AND BTLogger!
```

#### Benefits
- **✅ Zero code changes** to existing projects
- **✅ All ESP_LOG levels** supported (DEBUG, INFO, WARN, ERROR, VERBOSE)
- **✅ Automatic tag parsing** from ESP_LOG format
- **✅ Printf formatting** preserved (`ESP_LOGI("TAG", "Value: %d", val)`)
- **✅ Serial output unchanged** - still shows in terminal
- **✅ Works with libraries** that use ESP_LOG internally

### Manual API Reference (Original Method)

#### Initialization
```cpp
// Basic initialization
BTLoggerSender::begin();

// With custom device name
BTLoggerSender::begin("MyProject_v2.1");
```

#### Logging Functions
```cpp
// Direct logging
BTLoggerSender::log(BT_INFO, "TAG", "Message");

// Convenience functions
BTLoggerSender::debug("TAG", "Debug message");
BTLoggerSender::info("TAG", "Info message");  
BTLoggerSender::warn("TAG", "Warning message");
BTLoggerSender::error("TAG", "Error message");

// Macro versions (shorter syntax)
BT_LOG_DEBUG("TAG", "Debug message");
BT_LOG_INFO("TAG", "Info message");
BT_LOG_WARN("TAG", "Warning message");
BT_LOG_ERROR("TAG", "Error message");
```

#### Status Checking
```cpp
// Check if BTLogger is connected
if (BTLoggerSender::isConnected()) {
    // BTLogger is receiving logs
}

// Get statistics (ESP_LOG version only)
uint32_t totalLogs = BTLoggerSender::getLogCount();
uint32_t espLogs = BTLoggerSender::getESPLogCount();
```

### Integration Examples

#### ESP_LOG Integration Example (Weather Station)
```cpp
#include "BTLoggerSender_ESPLog.hpp" // ← Enhanced version

void setup() {
    Serial.begin(115200);
    BTLoggerSender::begin("WeatherStation_v2.1"); // ← Only addition needed!
    
    // All existing ESP_LOG calls automatically work:
    ESP_LOGI("SYSTEM", "Weather Station starting...");
    ESP_LOGI("SYSTEM", "Free heap: %d bytes", ESP.getFreeHeap());
    
    initializeSensors();
    connectWiFi();
}

void readSensors() {
    float temp = readTemperature();
    float humidity = readHumidity();
    
    // Standard ESP_LOG calls - automatically sent to BTLogger:
    ESP_LOGI("DHT22", "Temperature: %.1f°C", temp);
    ESP_LOGI("DHT22", "Humidity: %.1f%%", humidity);
    
    if (temp > 30.0) {
        ESP_LOGW("TEMP", "High temperature detected: %.1f°C", temp);
    }
}

void connectWiFi() {
    ESP_LOGI("WIFI", "Connecting to network...");
    
    WiFi.begin(ssid, password);
    
    if (WiFi.status() == WL_CONNECTED) {
        ESP_LOGI("WIFI", "Connected! IP: %s", WiFi.localIP().toString().c_str());
    } else {
        ESP_LOGE("WIFI", "Connection failed");
    }
}
```

#### Manual API Example (Custom Sensor)
```cpp
#include "BTLoggerSender.hpp" // ← Original version

void setup() {
    BTLoggerSender::begin("CustomSensor_v1.0");
    
    BT_LOG_INFO("SYSTEM", "Custom sensor system started");
}

void loop() {
    if (sensorReady()) {
        float value = readSensor();
        BT_LOG_INFO("SENSOR", "Reading: " + String(value));
        
        if (value > threshold) {
            BT_LOG_WARN("SENSOR", "Value exceeds threshold!");
        }
    } else {
        BT_LOG_ERROR("SENSOR", "Sensor not responding");
    }
    
    delay(1000);
}
```

## 🏗️ Architecture

### System Components

```
┌─────────────────────────────────────────┐
│              BTLogger ESP32             │
├─────────────────────────────────────────┤
│  LVGL UI Layer                         │
│  ├─ Main Screen (Log Viewer)           │
│  ├─ Menu System (SD Operations)        │
│  └─ Settings Screen (BT Config)        │
├─────────────────────────────────────────┤
│  Application Layer                     │
│  ├─ BluetoothManager (Central Mode)    │
│  ├─ LogManager (Display & Storage)     │
│  ├─ SDCardManager (File Operations)    │
│  └─ DeviceManager (Connection Handling)│
├─────────────────────────────────────────┤
│  Hardware Layer                        │
│  ├─ Display (ILI9341 + Touch)          │
│  ├─ SD Card (SPI Interface)            │
│  ├─ Input System (Buttons/Encoder)     │
│  └─ Bluetooth (ESP32 Built-in)         │
└─────────────────────────────────────────┘
```

### Communication Protocol

BTLogger uses BLE (Bluetooth Low Energy) with custom service:
- **Service UUID**: `12345678-1234-1234-1234-123456789abc`
- **Log Characteristic**: `87654321-4321-4321-4321-cba987654321`
- **Format**: Text-based for simplicity (binary format possible)
- **Discovery**: Auto-discovery by service UUID and device name patterns

### ESP_LOG Hook Architecture

```
┌─────────────────────────────────────────┐
│           Development ESP32             │
├─────────────────────────────────────────┤
│  Your Application Code                  │
│  ESP_LOGI("TAG", "Message %d", val)    │ ← No changes needed!
├─────────────────────────────────────────┤
│  ESP-IDF Logging System                │
│  esp_log_write() → vprintf()           │
├─────────────────────────────────────────┤
│  BTLoggerSender Hook                    │ ← Intercepts here
│  customVprintf() {                      │
│    originalVprintf(format, args);      │ ← Serial output preserved
│    parseAndSendToBTLogger(logs);       │ ← Also send to BTLogger
│  }                                      │
├─────────────────────────────────────────┤
│  BLE Transmission                       │
│  Send to BTLogger device               │
└─────────────────────────────────────────┘
```

## 🚧 Current Implementation Status

### ✅ Completed Features
- ✅ LVGL-based UI system
- ✅ Basic Bluetooth central functionality
- ✅ SD card logging infrastructure
- ✅ Log display with scrolling
- ✅ Connection status indicators
- ✅ BTLoggerSender library (Manual API)
- ✅ **ESP_LOG Integration** (Zero-code-change)
- ✅ Hardware LED status indicators
- ✅ Input system integration

### 🔄 In Progress Features
- 🔄 Complete BLE implementation
- 🔄 File browser UI screens
- 🔄 Device management interface
- 🔄 Settings configuration

### 📋 Planned Features
- 📋 Multiple device connections
- 📋 Log filtering and search
- 📋 Export functionality
- 📋 WiFi integration for remote access
- 📋 Time synchronization
- 📋 Advanced log analysis

## 🤝 Usage Scenarios

### IoT Development
- **Wireless Debugging**: Debug ESP32 projects without USB cables
- **Field Testing**: Monitor devices in remote locations
- **Sensor Networks**: Collect logs from multiple sensor nodes
- **Production Monitoring**: Track device behavior in deployed systems

### Educational Projects
- **Learning Tool**: Visualize program flow and debug issues
- **Demonstration**: Show real-time system behavior
- **Workshops**: Provide wireless logging for student projects
- **Prototyping**: Rapid iteration without cable management

### Legacy Project Integration
- **No Code Changes**: Add BTLogger to existing ESP32 projects instantly
- **Library Compatibility**: Works with any library using ESP_LOG
- **Drop-in Solution**: Just include header and add one line to setup()
- **Gradual Migration**: Can mix ESP_LOG and manual calls

## 📦 Dependencies

### BTLogger Device
- `lovyan03/LovyanGFX@^1.1.6` - Display driver
- `lvgl/lvgl@^8.3.11` - GUI framework  
- `ESP32-BLE-Arduino` - Bluetooth support
- `paulstoffregen/Encoder@^1.4.4` - Rotary encoder
- `thomasfredericks/Bounce2@^2.71` - Button debouncing

### Development Projects
- `ESP32-BLE-Arduino` - Only dependency needed
- Any existing libraries using ESP_LOG (automatically supported)

## 🔧 Troubleshooting

### Common Issues

#### BTLogger Not Discovering Devices
- Ensure both devices have Bluetooth enabled
- Check that development device is advertising the correct service UUID
- Verify BTLoggerSender::begin() is called in setup()

#### ESP_LOG Integration Not Working
- Ensure you're using `BTLoggerSender_ESPLog.hpp` (not the basic version)
- Check that ESP-IDF logging is enabled in your project
- Verify log level settings allow your ESP_LOG calls to execute

#### SD Card Not Working
- Check SD card is properly formatted (FAT32)
- Verify SD card connections match pin definitions
- Ensure adequate power supply for SD card operations

#### Display Issues
- Check display connections match ESP32_SPI_9341.h definitions
- Verify touch calibration if touch is not responsive
- Ensure adequate power supply for display

#### Bluetooth Connection Drops
- Check power management settings on development device
- Verify stable power supply to both devices
- Ensure devices are within reasonable range

### Debug Steps
1. **Check Serial Output** - Both devices provide detailed serial logging
2. **Test ESP_LOG Hook** - Look for "ESP_LOG integration active" message
3. **Verify Hardware** - Test individual components (display, SD, BLE)
4. **Test with Example** - Use provided example files
5. **Check Power** - Ensure stable 5V supply for both devices

## 🆚 Integration Comparison

| Feature | ESP_LOG Integration | Manual API |
|---------|-------------------|------------|
| **Code Changes** | Zero | Minimal |
| **Existing Projects** | Drop-in | Need updates |
| **Library Compatibility** | Automatic | Manual |
| **Learning Curve** | None | Minimal |
| **Control Level** | Automatic | Full |
| **Performance** | Slight overhead | Optimized |
| **Best For** | Existing projects | New projects |

## 🏆 Credits

Built upon the excellent foundation:
- Original template from [esp32-smartdisplay-demo](https://github.com/rzeldent/esp32-smartdisplay-demo)
- Display drivers by [LovyanGFX](https://github.com/lovyan03/LovyanGFX)
- GUI framework by [LVGL](https://github.com/lvgl/lvgl)
- ESP_LOG integration inspired by ESP-IDF logging architecture

## 📄 License

This project maintains the same license as the original template. See the original repository for license details.

---

**BTLogger** - Making ESP32 development debugging wireless and visual! 📡📱

*Now with zero-code-change ESP_LOG integration!* ✨
