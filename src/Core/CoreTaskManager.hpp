#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

namespace BTLogger {
namespace Core {

// Forward declarations
class BluetoothManager;
class SDCardManager;

// Task priorities
#define COMMUNICATIONS_TASK_PRIORITY 2
#define UI_TASK_PRIORITY 3
#define TASK_STACK_SIZE 8192

// Message types for inter-core communication
enum MessageType {
    MSG_LOG_RECEIVED,
    MSG_DEVICE_CONNECTION,
    MSG_UI_EVENT,
    MSG_FILE_OPERATION,
    MSG_SHUTDOWN
};

// Message structure for inter-core communication
struct CoreMessage {
    MessageType type;
    String data1;
    String data2;
    uint32_t value1;
    uint32_t value2;

    CoreMessage() : type(MSG_SHUTDOWN), value1(0), value2(0) {}
    CoreMessage(MessageType t, const String& d1 = "", const String& d2 = "", uint32_t v1 = 0, uint32_t v2 = 0)
        : type(t), data1(d1), data2(d2), value1(v1), value2(v2) {}
};

/**
 * CoreTaskManager manages multithreaded operations across ESP32 cores
 * Core 0: Communications (Bluetooth, SD Card)
 * Core 1: UI (Display, Touch, Menu)
 */
class CoreTaskManager {
   public:
    CoreTaskManager();
    ~CoreTaskManager();

    // Core lifecycle
    bool initialize();
    void start();
    void stop();
    bool isRunning() const { return running; }

    // Inter-core communication
    bool sendToUI(const CoreMessage& message);
    bool sendToCommunications(const CoreMessage& message);

    // Manager access (thread-safe)
    BluetoothManager* getBluetoothManager() const { return bluetoothManager; }
    SDCardManager* getSDCardManager() const { return sdCardManager; }

    // Status
    bool isCommunicationsTaskRunning() const { return communicationsTaskRunning; }
    bool isUITaskRunning() const { return uiTaskRunning; }

   private:
    // Core managers
    BluetoothManager* bluetoothManager;
    SDCardManager* sdCardManager;

    // Task handles
    TaskHandle_t communicationsTaskHandle;
    TaskHandle_t uiTaskHandle;

    // Synchronization
    SemaphoreHandle_t managerMutex;
    QueueHandle_t uiMessageQueue;
    QueueHandle_t communicationsMessageQueue;

    // State
    bool running;
    bool communicationsTaskRunning;
    bool uiTaskRunning;

    // Task functions (static for FreeRTOS)
    static void communicationsTask(void* parameter);
    static void uiTask(void* parameter);

    // Internal task loops
    void runCommunicationsLoop();
    void runUILoop();

    // Message handlers
    void handleUIMessage(const CoreMessage& message);
    void handleCommunicationsMessage(const CoreMessage& message);

    // Cleanup
    void cleanupTasks();
};

}  // namespace Core
}  // namespace BTLogger