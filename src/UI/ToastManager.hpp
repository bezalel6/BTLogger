#pragma once

#include <Arduino.h>
#include <LovyanGFX.hpp>

namespace BTLogger {
namespace UI {

/**
 * ToastManager provides non-intrusive notifications
 */
class ToastManager {
   public:
    enum ToastType {
        INFO,
        SUCCESS,
        WARNING,
        ERROR
    };

    // Initialization
    static void initialize(lgfx::LGFX_Device& display);
    static bool isInitialized() { return initialized; }

    // Main update loop
    static void update();

    // Show toasts
    static void showInfo(const String& message);
    static void showSuccess(const String& message);
    static void showWarning(const String& message);
    static void showError(const String& message);

   private:
    static bool initialized;
    static lgfx::LGFX_Device* lcd;
    static String currentMessage;
    static ToastType currentType;
    static unsigned long showTime;
    static bool visible;
    static const unsigned long TOAST_DURATION = 1500;  // 1.5 seconds

    static void showToast(const String& message, ToastType type);
    static void drawToast();
    static uint16_t getToastColor(ToastType type);
};

}  // namespace UI
}  // namespace BTLogger