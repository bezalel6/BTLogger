#pragma once

#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <functional>

namespace BTLogger {
namespace UI {

/**
 * Base class for all screens in the BTLogger UI system
 */
class Screen {
   public:
    using NavigationCallback = std::function<void(const String& screenName)>;
    using BackCallback = std::function<void()>;

    Screen(const String& name) : screenName(name), lcd(nullptr), active(false) {}
    virtual ~Screen() = default;

    // Lifecycle methods
    virtual void initialize(lgfx::LGFX_Device& display) { lcd = &display; }
    virtual void activate() {
        active = true;
        needsRedraw = true;
    }
    virtual void deactivate() { active = false; }
    virtual void cleanup() {}

    // Main loop methods
    virtual void update() = 0;
    virtual void handleTouch(int x, int y, bool touched) = 0;

    // Navigation callbacks
    void setNavigationCallback(NavigationCallback callback) { navigationCallback = callback; }
    void setBackCallback(BackCallback callback) { backCallback = callback; }

    // State
    bool isActive() const { return active; }
    String getName() const { return screenName; }
    void markForRedraw() { needsRedraw = true; }

   protected:
    String screenName;
    lgfx::LGFX_Device* lcd;
    bool active;
    bool needsRedraw = true;

    NavigationCallback navigationCallback;
    BackCallback backCallback;

    // Helper method to navigate to another screen
    void navigateTo(const String& screenName) {
        if (navigationCallback) {
            navigationCallback(screenName);
        }
    }

    // Helper method to go back
    void goBack() {
        if (backCallback) {
            backCallback();
        }
    }

   public:
    // Common UI constants
    static const int BACK_BUTTON_WIDTH = 80;
    static const int BACK_BUTTON_HEIGHT = 35;
    static const int HEADER_HEIGHT = 50;
    static const int FOOTER_HEIGHT = 25;
};

}  // namespace UI
}  // namespace BTLogger