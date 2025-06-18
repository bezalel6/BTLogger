# BTLogger Multithreaded Implementation

## Overview
This document describes the implementation of multithreaded functionality in BTLogger, splitting UI and communications onto separate ESP32 cores, along with various UI improvements.

## Architecture Changes

### Core Task Separation
- **Core 0 (Communications)**: Bluetooth operations, SD Card I/O, file operations
- **Core 1 (UI)**: Touch handling, display rendering, menu navigation, toast notifications

### New Components

#### CoreTaskManager (`src/Core/CoreTaskManager.hpp/.cpp`)
Central coordinator for multithreaded operations:
- Manages FreeRTOS tasks on different cores
- Provides thread-safe inter-core communication via message queues
- Handles proper task lifecycle and cleanup
- Ensures thread-safe access to shared resources

#### Inter-Core Communication
```cpp
// Message structure for core communication
struct CoreMessage {
    MessageType type;
    String data1, data2;
    uint32_t value1, value2;
};

// Message types
enum MessageType {
    MSG_LOG_RECEIVED,        // Log data from communications to UI
    MSG_DEVICE_CONNECTION,   // Connection status updates
    MSG_UI_EVENT,           // UI interactions
    MSG_FILE_OPERATION,     // File system operations
    MSG_SHUTDOWN            // Task termination
};
```

### Task Implementation

#### Communications Task (Core 0)
- Runs `BluetoothManager::update()` for BLE operations
- Handles file operations via `SDCardManager`
- Processes inter-core messages
- Updates at ~1000Hz with 1ms delays

#### UI Task (Core 1)
- Updates `TouchManager`, `ToastManager`, `MenuManager`
- Processes UI-related messages
- Handles display rendering
- Updates at 100Hz with 10ms delays

### Synchronization Mechanisms
- **Mutex**: Thread-safe access to manager instances
- **Message Queues**: Asynchronous communication between cores
- **Timeout-based Operations**: Prevents deadlocks

## UI Improvements

### 1. Fixed Display Flickering
**Problem**: MenuManager was redrawing the entire screen every second
**Solution**: Implemented dirty state tracking
```cpp
static bool needsRedraw = true;
static void markForRedraw();

// Only redraw when needed
if (needsRedraw) {
    drawMainMenu();
    needsRedraw = false;
}
```

### 2. Improved Scale for Better Readability
**Problem**: Default scale (1.0x) was too small for 240x320 display
**Solution**: Increased default scales
```cpp
const float SCALE_NORMAL = 1.3f;  // Was 1.0f
const float SCALE_LARGE = 1.5f;   // Was 1.2f  
const float SCALE_HUGE = 1.8f;    // Was 1.5f
```

### 3. Enhanced Button Appearance
**Problem**: Buttons looked flat without clear borders
**Solution**: Added 3D visual effects
- Drop shadow for depth
- Raised border effect for normal state
- Pressed state with inverted border
- Better color palette with light/dark variants

```cpp
// 3D border effect
if (enabled) {
    if (pressed) {
        // Pressed state - darker border
        lcd.drawRoundRect(x, y, width, height, UIScale::scale(5), Colors::GRAY);
        lcd.drawRoundRect(x + 1, y + 1, width - 2, height - 2, UIScale::scale(4), Colors::WHITE);
    } else {
        // Normal state - raised border effect with shadow
        lcd.fillRoundRect(x + 2, y + 2, width, height, UIScale::scale(5), Colors::BLACK); // Shadow
        lcd.drawRoundRect(x, y, width, height, UIScale::scale(5), Colors::WHITE);
        lcd.drawRoundRect(x + 1, y + 1, width - 2, height - 2, UIScale::scale(4), Colors::LIGHT_GRAY);
    }
}
```

### 4. Enhanced Menu Design
- Colored title with underline
- Better text sizing with proper scaling
- Status line at bottom
- Improved spacing and layout

## Performance Benefits

### Memory Efficiency
- Communications and UI operations isolated
- Reduced contention for shared resources
- Better memory management per core

### Responsiveness
- UI remains responsive during heavy Bluetooth operations
- File I/O doesn't block touch interactions
- Smoother display updates

### System Stability
- Proper task cleanup on shutdown
- Watchdog-safe operation with strategic delays
- Thread-safe resource access

## Integration with BTLoggerApp

The main application now uses `CoreTaskManager`:
```cpp
// Initialization
coreTaskManager = new Core::CoreTaskManager();
coreTaskManager->initialize();

// Start multithreaded operation
coreTaskManager->start();

// Light main loop - heavy work is in tasks
void BTLoggerApp::update() {
    // Only LED updates and basic maintenance
    if (currentTime - lastUpdate > 100) {  // 10Hz
        updateLEDs();
        lastUpdate = currentTime;
    }
    delay(1);  // Prevent watchdog issues
}
```

## Thread Safety Considerations

### Shared Resource Access
- All manager access protected by mutex
- Message queues for asynchronous communication
- No direct cross-core memory sharing

### Callback Handling
- Bluetooth callbacks execute on communications core
- UI updates sent via message queue to UI core
- No direct UI calls from communications core

## Configuration

### Task Priorities
```cpp
#define COMMUNICATIONS_TASK_PRIORITY 2
#define UI_TASK_PRIORITY 3
#define TASK_STACK_SIZE 8192
```

### Message Queue Sizes
- UI Message Queue: 10 messages
- Communications Message Queue: 10 messages
- 100ms timeout for message sending

## Future Enhancements

1. **Dynamic Load Balancing**: Adjust task priorities based on workload
2. **Performance Monitoring**: Track task CPU usage and memory
3. **Advanced UI Animations**: Leverage dedicated UI core for smooth effects
4. **Background File Processing**: Large file operations without UI blocking

## Testing Recommendations

1. **Stress Testing**: Multiple device connections + heavy UI interaction
2. **Memory Testing**: Long-running sessions to check for leaks
3. **Timing Analysis**: Verify task update frequencies
4. **Crash Recovery**: Ensure graceful handling of task failures

This implementation provides a solid foundation for a responsive, professional BTLogger interface with proper separation of concerns and excellent performance characteristics. 