#pragma once

#include <Arduino.h>
#include <LovyanGFX.hpp>

namespace BTLogger {
namespace UI {

/**
 * CriticalErrorHandler provides BSOD-style error recovery
 */
class CriticalErrorHandler {
   public:
    // Initialization
    static void initialize(lgfx::LGFX_Device& display);
    static bool isInitialized() { return initialized; }

    // Error handling
    static void showCriticalError(const String& error, const String& details = "");
    static void handleFatalError(const String& error);

   private:
    static bool initialized;
    static lgfx::LGFX_Device* lcd;
};

}  // namespace UI
}  // namespace BTLogger