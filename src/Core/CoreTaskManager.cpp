#include "CoreTaskManager.hpp"
#include "BluetoothManager.hpp"
#include "SDCardManager.hpp"
#include "../UI/TouchManager.hpp"
#include "../UI/ToastManager.hpp"
#include "../UI/ScreenManager.hpp"
#include "../UI/UIScale.hpp"

namespace BTLogger {
namespace Core {

CoreTaskManager::CoreTaskManager()
    : bluetoothManager(nullptr), sdCardManager(nullptr), communicationsTaskHandle(nullptr), uiTaskHandle(nullptr), managerMutex(nullptr), uiMessageQueue(nullptr), communicationsMessageQueue(nullptr), running(false), communicationsTaskRunning(false), uiTaskRunning(false) {
}

CoreTaskManager::~CoreTaskManager() {
    stop();
}

bool CoreTaskManager::initialize() {
    Serial.println("Initializing CoreTaskManager...");

    // Create synchronization objects
    managerMutex = xSemaphoreCreateMutex();
    if (!managerMutex) {
        Serial.println("Failed to create manager mutex");
        return false;
    }

    // Create message queues
    uiMessageQueue = xQueueCreate(10, sizeof(CoreMessage));
    communicationsMessageQueue = xQueueCreate(10, sizeof(CoreMessage));

    if (!uiMessageQueue || !communicationsMessageQueue) {
        Serial.println("Failed to create message queues");
        return false;
    }

    // Initialize managers (on main core for now)
    xSemaphoreTake(managerMutex, portMAX_DELAY);

    sdCardManager = new SDCardManager();
    if (!sdCardManager->initialize()) {
        Serial.println("WARNING: SD Card initialization failed - logging disabled");
    }

    bluetoothManager = new BluetoothManager();
    if (!bluetoothManager->initialize()) {
        Serial.println("ERROR: Bluetooth initialization failed");
        xSemaphoreGive(managerMutex);
        return false;
    }

    xSemaphoreGive(managerMutex);

    Serial.println("CoreTaskManager initialized successfully");
    return true;
}

void CoreTaskManager::start() {
    if (running) {
        return;
    }

    Serial.println("Starting CoreTaskManager tasks...");

    // Create communications task on Core 0
    xTaskCreatePinnedToCore(
        communicationsTask,
        "CommunicationsTask",
        TASK_STACK_SIZE,
        this,
        COMMUNICATIONS_TASK_PRIORITY,
        &communicationsTaskHandle,
        0  // Core 0
    );

    // Create UI task on Core 1
    xTaskCreatePinnedToCore(
        uiTask,
        "UITask",
        TASK_STACK_SIZE,
        this,
        UI_TASK_PRIORITY,
        &uiTaskHandle,
        1  // Core 1
    );

    running = true;
    Serial.println("CoreTaskManager tasks started");
}

void CoreTaskManager::stop() {
    if (!running) {
        return;
    }

    Serial.println("Stopping CoreTaskManager...");

    running = false;

    // Send shutdown messages
    CoreMessage shutdownMsg(MSG_SHUTDOWN);
    sendToUI(shutdownMsg);
    sendToCommunications(shutdownMsg);

    // Wait for tasks to finish
    unsigned long timeout = millis() + 5000;  // 5 second timeout
    while ((communicationsTaskRunning || uiTaskRunning) && millis() < timeout) {
        delay(100);
    }

    // Force cleanup if needed
    cleanupTasks();

    // Cleanup managers
    xSemaphoreTake(managerMutex, portMAX_DELAY);
    delete bluetoothManager;
    delete sdCardManager;
    bluetoothManager = nullptr;
    sdCardManager = nullptr;
    xSemaphoreGive(managerMutex);

    // Cleanup synchronization objects
    if (uiMessageQueue) {
        vQueueDelete(uiMessageQueue);
        uiMessageQueue = nullptr;
    }
    if (communicationsMessageQueue) {
        vQueueDelete(communicationsMessageQueue);
        communicationsMessageQueue = nullptr;
    }
    if (managerMutex) {
        vSemaphoreDelete(managerMutex);
        managerMutex = nullptr;
    }

    Serial.println("CoreTaskManager stopped");
}

bool CoreTaskManager::sendToUI(const CoreMessage& message) {
    if (!uiMessageQueue) return false;
    return xQueueSend(uiMessageQueue, &message, pdMS_TO_TICKS(100)) == pdTRUE;
}

bool CoreTaskManager::sendToCommunications(const CoreMessage& message) {
    if (!communicationsMessageQueue) return false;
    return xQueueSend(communicationsMessageQueue, &message, pdMS_TO_TICKS(100)) == pdTRUE;
}

void CoreTaskManager::communicationsTask(void* parameter) {
    CoreTaskManager* manager = static_cast<CoreTaskManager*>(parameter);
    manager->runCommunicationsLoop();
}

void CoreTaskManager::uiTask(void* parameter) {
    CoreTaskManager* manager = static_cast<CoreTaskManager*>(parameter);
    manager->runUILoop();
}

void CoreTaskManager::runCommunicationsLoop() {
    Serial.println("Communications task started on Core 0");
    communicationsTaskRunning = true;

    CoreMessage message;
    const TickType_t messageTimeout = pdMS_TO_TICKS(10);  // 10ms timeout

    while (running) {
        // Process incoming messages
        if (xQueueReceive(communicationsMessageQueue, &message, messageTimeout) == pdTRUE) {
            if (message.type == MSG_SHUTDOWN) {
                Serial.println("Communications task received shutdown message");
                break;
            }
            handleCommunicationsMessage(message);
        }

        // Update managers (thread-safe)
        xSemaphoreTake(managerMutex, portMAX_DELAY);
        if (bluetoothManager) {
            bluetoothManager->update();
        }
        xSemaphoreGive(managerMutex);

        // Small delay to prevent watchdog issues
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    communicationsTaskRunning = false;
    Serial.println("Communications task ended");
    vTaskDelete(nullptr);
}

void CoreTaskManager::runUILoop() {
    Serial.println("UI task started on Core 1");
    uiTaskRunning = true;

    CoreMessage message;
    const TickType_t messageTimeout = pdMS_TO_TICKS(10);  // 10ms timeout

    while (running) {
        // Process incoming messages
        if (xQueueReceive(uiMessageQueue, &message, messageTimeout) == pdTRUE) {
            if (message.type == MSG_SHUTDOWN) {
                Serial.println("UI task received shutdown message");
                break;
            }
            handleUIMessage(message);
        }

        // Update UI systems
        UI::TouchManager::update();
        UI::ToastManager::update();
        UI::ScreenManager::update();

        // Handle touch input for screen manager
        UI::TouchManager::TouchPoint touch = UI::TouchManager::getTouch();
        UI::ScreenManager::handleTouch(touch.x, touch.y, touch.pressed);

        // Small delay to prevent watchdog issues
        vTaskDelay(pdMS_TO_TICKS(20));  // 50Hz UI update rate
    }

    uiTaskRunning = false;
    Serial.println("UI task ended");
    vTaskDelete(nullptr);
}

void CoreTaskManager::handleUIMessage(const CoreMessage& message) {
    switch (message.type) {
        case MSG_LOG_RECEIVED:
            // Show toast for important logs
            if (message.value1 >= 2) {  // WARN and ERROR levels
                String levelStr = (message.value1 == 2) ? "WARN" : "ERROR";
                String toastMsg = message.data1 + " " + levelStr + ": " + message.data2;
                if (message.value1 == 3) {
                    UI::ToastManager::showError(toastMsg);
                } else {
                    UI::ToastManager::showWarning(toastMsg);
                }
            }
            break;

        case MSG_DEVICE_CONNECTION:
            // Device connection status is now shown in the status footer
            // No need for toasts here anymore
            break;

        default:
            break;
    }
}

void CoreTaskManager::handleCommunicationsMessage(const CoreMessage& message) {
    switch (message.type) {
        case MSG_FILE_OPERATION:
            // Handle file operations on communications core
            xSemaphoreTake(managerMutex, portMAX_DELAY);
            if (sdCardManager) {
                if (message.data1 == "load") {
                    std::vector<String> lines = sdCardManager->loadLogFile(message.data2);
                    // Send result back to UI
                    CoreMessage result(MSG_UI_EVENT, "file_loaded", String(lines.size()));
                    sendToUI(result);
                } else if (message.data1 == "delete") {
                    bool success = sdCardManager->deleteFile(message.data2);
                    CoreMessage result(MSG_UI_EVENT, "file_deleted", success ? "success" : "failed");
                    sendToUI(result);
                }
            }
            xSemaphoreGive(managerMutex);
            break;

        default:
            break;
    }
}

void CoreTaskManager::cleanupTasks() {
    if (communicationsTaskHandle && communicationsTaskRunning) {
        vTaskDelete(communicationsTaskHandle);
        communicationsTaskHandle = nullptr;
        communicationsTaskRunning = false;
    }

    if (uiTaskHandle && uiTaskRunning) {
        vTaskDelete(uiTaskHandle);
        uiTaskHandle = nullptr;
        uiTaskRunning = false;
    }
}

}  // namespace Core
}  // namespace BTLogger