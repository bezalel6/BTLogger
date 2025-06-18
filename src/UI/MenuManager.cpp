#include "MenuManager.hpp"
#include "UIScale.hpp"
#include "TouchManager.hpp"
#include "Widgets/Button.hpp"
#include <algorithm>

namespace BTLogger {
namespace UI {

// Static member definitions
bool MenuManager::initialized = false;
lgfx::LGFX_Device* MenuManager::lcd = nullptr;
bool MenuManager::needsRedraw = true;
unsigned long MenuManager::lastInteraction = 0;
bool MenuManager::lastTouchState = false;

// Scrolling
int MenuManager::scrollOffset = 0;
int MenuManager::maxScrollOffset = 0;

// Status footer
String MenuManager::statusText = "Touch buttons to navigate";

// Menu buttons
Widgets::Button* MenuManager::logViewerButton = nullptr;
Widgets::Button* MenuManager::deviceManagerButton = nullptr;
Widgets::Button* MenuManager::fileBrowserButton = nullptr;
Widgets::Button* MenuManager::settingsButton = nullptr;
Widgets::Button* MenuManager::systemInfoButton = nullptr;

void MenuManager::initialize(lgfx::LGFX_Device& display) {
    if (initialized) {
        return;
    }

    lcd = &display;

    // Create menu buttons with proper spacing and centering
    int buttonWidth = UIScale::scale(200);
    int buttonHeight = UIScale::scale(35);
    int buttonX = (lcd->width() - buttonWidth) / 2;  // Center horizontally
    int startY = UIScale::scale(70);
    int buttonSpacing = UIScale::scale(45);

    logViewerButton = new Widgets::Button(*lcd, buttonX, startY, buttonWidth, buttonHeight, "LOG VIEWER");
    logViewerButton->setCallback([]() {
        Serial.println("Log Viewer selected");
        setStatusText("Opening Log Viewer...");
        // TODO: Navigate to log viewer screen
    });
    Serial.printf("Log Viewer button: x=%d, y=%d, w=%d, h=%d\n", buttonX, startY, buttonWidth, buttonHeight);

    deviceManagerButton = new Widgets::Button(*lcd, buttonX, startY + buttonSpacing, buttonWidth, buttonHeight, "DEVICE MANAGER");
    deviceManagerButton->setCallback([]() {
        Serial.println("Device Manager selected");
        setStatusText("Opening Device Manager...");
        // TODO: Navigate to device manager screen
    });
    Serial.printf("Device Manager button: x=%d, y=%d, w=%d, h=%d\n", buttonX, startY + buttonSpacing, buttonWidth, buttonHeight);

    fileBrowserButton = new Widgets::Button(*lcd, buttonX, startY + buttonSpacing * 2, buttonWidth, buttonHeight, "FILE BROWSER");
    fileBrowserButton->setCallback([]() {
        Serial.println("File Browser selected");
        setStatusText("Opening File Browser...");
        // TODO: Navigate to file browser screen
    });
    Serial.printf("File Browser button: x=%d, y=%d, w=%d, h=%d\n", buttonX, startY + buttonSpacing * 2, buttonWidth, buttonHeight);

    settingsButton = new Widgets::Button(*lcd, buttonX, startY + buttonSpacing * 3, buttonWidth, buttonHeight, "SETTINGS");
    settingsButton->setCallback([]() {
        Serial.println("Settings selected");
        setStatusText("Opening Settings...");
        // TODO: Navigate to settings screen
    });
    Serial.printf("Settings button: x=%d, y=%d, w=%d, h=%d\n", buttonX, startY + buttonSpacing * 3, buttonWidth, buttonHeight);

    systemInfoButton = new Widgets::Button(*lcd, buttonX, startY + buttonSpacing * 4, buttonWidth, buttonHeight, "SYSTEM INFO");
    systemInfoButton->setCallback([]() {
        Serial.println("=== System Info / Touch Debug ===");
        TouchManager::showTouchDebugInfo();
        setStatusText("Touch debug info printed to serial");

        // TODO: Add option to recalibrate touch if needed
        // TouchManager::resetCalibration();

        Serial.println("To recalibrate touch, uncomment resetCalibration() call");
    });
    Serial.printf("System Info button: x=%d, y=%d, w=%d, h=%d\n", buttonX, startY + buttonSpacing * 4, buttonWidth, buttonHeight);

    initialized = true;
    needsRedraw = true;

    // Calculate scroll limits
    maxScrollOffset = std::max(0, BUTTON_COUNT - VISIBLE_BUTTONS);

    Serial.println("MenuManager initialized with centered buttons");
    Serial.printf("Scroll setup: %d buttons, %d visible, maxScroll=%d\n", BUTTON_COUNT, VISIBLE_BUTTONS, maxScrollOffset);
}

void MenuManager::update() {
    if (!initialized) {
        return;
    }

    // Only redraw when needed
    if (needsRedraw) {
        drawMainMenu();
        needsRedraw = false;
        lastInteraction = millis();
    }

    // Update all buttons
    if (logViewerButton) logViewerButton->update();
    if (deviceManagerButton) deviceManagerButton->update();
    if (fileBrowserButton) fileBrowserButton->update();
    if (settingsButton) settingsButton->update();
    if (systemInfoButton) systemInfoButton->update();

    // Handle touch input for buttons and scrolling
    TouchManager::TouchPoint touch = TouchManager::getTouch();
    bool currentlyTouched = TouchManager::isTouched();
    bool wasTapped = TouchManager::wasTapped();

    if (currentlyTouched || lastTouchState) {  // Process touch state changes
        bool buttonHandled = false;

        // Check for scroll gestures first (if we have scrollable content)
        if (wasTapped && maxScrollOffset > 0) {
            int lineY = UIScale::scale(15) + UIScale::scale(30);  // Title line
            int footerHeight = UIScale::scale(25);
            int scrollZoneHeight = UIScale::scale(30);

            // Touch in top scroll zone - scroll up
            if (touch.y >= lineY && touch.y < lineY + scrollZoneHeight && touch.x > lcd->width() - UIScale::scale(30)) {
                if (scrollOffset > 0) {
                    scrollUp();
                    setStatusText("Scrolled up");
                }
                TouchManager::clearTap();
                lastTouchState = currentlyTouched;
                return;
            }

            // Touch in bottom scroll zone - scroll down
            if (touch.y >= lcd->height() - footerHeight - scrollZoneHeight && touch.y < lcd->height() - footerHeight && touch.x > lcd->width() - UIScale::scale(30)) {
                if (scrollOffset < maxScrollOffset) {
                    scrollDown();
                    setStatusText("Scrolled down");
                }
                TouchManager::clearTap();
                lastTouchState = currentlyTouched;
                return;
            }
        }

        // Handle button touches
        if (logViewerButton) buttonHandled |= logViewerButton->handleTouch(touch.x, touch.y, currentlyTouched);
        if (deviceManagerButton) buttonHandled |= deviceManagerButton->handleTouch(touch.x, touch.y, currentlyTouched);
        if (fileBrowserButton) buttonHandled |= fileBrowserButton->handleTouch(touch.x, touch.y, currentlyTouched);
        if (settingsButton) buttonHandled |= settingsButton->handleTouch(touch.x, touch.y, currentlyTouched);
        if (systemInfoButton) buttonHandled |= systemInfoButton->handleTouch(touch.x, touch.y, currentlyTouched);

        lastTouchState = currentlyTouched;
    }
}

void MenuManager::markForRedraw() {
    needsRedraw = true;
}

void MenuManager::drawMainMenu() {
    if (!lcd) return;

    // Clear screen only once
    lcd->fillScreen(0x0000);  // Black

    // Title with better styling
    lcd->setTextColor(0x07FF);  // Cyan
    lcd->setTextSize(UIScale::scale(3));
    int titleY = UIScale::scale(15);
    lcd->setCursor(UIScale::scale(30), titleY);
    lcd->print("BTLogger");

    // Draw a line under title using actual display width
    int lineY = titleY + UIScale::scale(30);
    lcd->drawFastHLine(UIScale::scale(10), lineY, lcd->width() - UIScale::scale(20), 0x07FF);

    // Calculate visible button area (leave space for footer)
    int footerHeight = UIScale::scale(25);
    int startY = UIScale::scale(70);
    int buttonSpacing = UIScale::scale(45);

    // Store all buttons in array for easier scrolling management
    Widgets::Button* buttons[] = {logViewerButton, deviceManagerButton, fileBrowserButton, settingsButton, systemInfoButton};

    // Update button positions based on scroll offset and draw visible ones
    for (int i = 0; i < BUTTON_COUNT; i++) {
        if (buttons[i]) {
            // Calculate new Y position based on scroll
            int newY = startY + (i * buttonSpacing) - (scrollOffset * buttonSpacing);

            // Update button position
            int buttonX = (lcd->width() - UIScale::scale(200)) / 2;
            buttons[i]->setPosition(buttonX, newY);

            // Only draw if visible on screen
            if (newY >= lineY + UIScale::scale(10) && newY < lcd->height() - footerHeight) {
                buttons[i]->draw();
            }
        }
    }

    // Draw scroll indicators if needed
    if (maxScrollOffset > 0) {
        int indicatorX = lcd->width() - UIScale::scale(15);

        // Up arrow
        if (scrollOffset > 0) {
            lcd->setTextColor(0xFFFF);
            lcd->setTextSize(1);
            lcd->setCursor(indicatorX, lineY + UIScale::scale(10));
            lcd->print("^");
        }

        // Down arrow
        if (scrollOffset < maxScrollOffset) {
            lcd->setTextColor(0xFFFF);
            lcd->setTextSize(1);
            lcd->setCursor(indicatorX, lcd->height() - footerHeight - UIScale::scale(15));
            lcd->print("v");
        }
    }

    // Draw footer with status
    int footerY = lcd->height() - footerHeight;
    lcd->drawFastHLine(0, footerY, lcd->width(), 0x8410);  // Gray line

    lcd->setTextColor(0x8410);  // Gray
    lcd->setTextSize(UIScale::scale(1));
    lcd->setCursor(UIScale::scale(5), footerY + UIScale::scale(5));
    lcd->print(statusText.substring(0, 35));  // Limit length to fit screen
}

void MenuManager::cleanup() {
    delete logViewerButton;
    delete deviceManagerButton;
    delete fileBrowserButton;
    delete settingsButton;
    delete systemInfoButton;

    logViewerButton = nullptr;
    deviceManagerButton = nullptr;
    fileBrowserButton = nullptr;
    settingsButton = nullptr;
    systemInfoButton = nullptr;
}

void MenuManager::setStatusText(const String& status) {
    statusText = status;
    markForRedraw();
}

void MenuManager::clearStatus() {
    statusText = "Touch buttons to navigate";
    markForRedraw();
}

void MenuManager::scrollUp() {
    if (scrollOffset > 0) {
        scrollOffset--;
        markForRedraw();
    }
}

void MenuManager::scrollDown() {
    if (scrollOffset < maxScrollOffset) {
        scrollOffset++;
        markForRedraw();
    }
}

}  // namespace UI
}  // namespace BTLogger