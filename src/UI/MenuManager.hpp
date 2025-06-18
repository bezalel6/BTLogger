#pragma once

#include <Arduino.h>
#include <LovyanGFX.hpp>

namespace BTLogger {
namespace UI {

/**
 * MenuManager provides basic menu navigation
 */
class MenuManager {
   public:
    // Initialization
    static void initialize(lgfx::LGFX_Device& display);
    static bool isInitialized() { return initialized; }

    // Main update loop
    static void update();

   private:
    static bool initialized;
    static lgfx::LGFX_Device* lcd;

    static void drawMainMenu();
};

}  // namespace UI
}  // namespace BTLogger