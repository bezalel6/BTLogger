#pragma once

#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <map>
#include <vector>
#include "Screen.hpp"

namespace BTLogger {
namespace UI {

/**
 * ScreenManager handles navigation between different UI screens
 */
class ScreenManager {
   public:
    static void initialize(lgfx::LGFX_Device& display);
    static void cleanup();

    // Screen management
    static void registerScreen(Screen* screen);
    static void navigateTo(const String& screenName);
    static void goBack();
    static void setStatusText(const String& status);

    // Main loop
    static void update();
    static void handleTouch(int x, int y, bool touched);

    // State
    static String getCurrentScreenName();
    static bool isInitialized() { return initialized; }

   private:
    static bool initialized;
    static lgfx::LGFX_Device* lcd;
    static std::map<String, Screen*> screens;
    static std::vector<String> navigationStack;
    static Screen* currentScreen;
    static String statusText;
    static bool footerNeedsRedraw;

    static void drawStatusFooter();
    static void switchToScreen(const String& screenName);
};

}  // namespace UI
}  // namespace BTLogger