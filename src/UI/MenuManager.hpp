#pragma once

#include <Arduino.h>
#include <LovyanGFX.hpp>

namespace BTLogger {
namespace UI {

// Forward declaration
namespace Widgets {
class Button;
}

/**
 * MenuManager provides basic menu navigation with optimized rendering
 */
class MenuManager {
   public:
    // Initialization
    static void initialize(lgfx::LGFX_Device& display);
    static bool isInitialized() { return initialized; }

    // Main update loop
    static void update();

    // Force redraw
    static void markForRedraw();

    // Cleanup
    static void cleanup();

    // Status footer
    static void setStatusText(const String& status);
    static void clearStatus();

    // Scrolling
    static void scrollUp();
    static void scrollDown();

   private:
    static bool initialized;
    static lgfx::LGFX_Device* lcd;
    static bool needsRedraw;
    static unsigned long lastInteraction;
    static bool lastTouchState;

    // Scrolling
    static int scrollOffset;
    static int maxScrollOffset;
    static const int BUTTON_COUNT = 5;
    static const int VISIBLE_BUTTONS = 4;  // How many buttons fit on screen

    // Status footer
    static String statusText;

    // Menu buttons
    static Widgets::Button* logViewerButton;
    static Widgets::Button* deviceManagerButton;
    static Widgets::Button* fileBrowserButton;
    static Widgets::Button* settingsButton;
    static Widgets::Button* systemInfoButton;

    static void drawMainMenu();
};

}  // namespace UI
}  // namespace BTLogger