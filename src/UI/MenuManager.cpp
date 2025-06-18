#include "MenuManager.hpp"
#include "UIScale.hpp"
#include "TouchManager.hpp"

namespace BTLogger {
namespace UI {

// Static member definitions
bool MenuManager::initialized = false;
lgfx::LGFX_Device* MenuManager::lcd = nullptr;

void MenuManager::initialize(lgfx::LGFX_Device& display) {
    if (initialized) {
        return;
    }

    lcd = &display;
    initialized = true;

    // Draw initial menu
    drawMainMenu();

    Serial.println("MenuManager initialized");
}

void MenuManager::update() {
    if (!initialized) {
        return;
    }

    // Simple menu interaction would go here
    // For now, just redraw if needed
    static unsigned long lastDraw = 0;
    if (millis() - lastDraw > 1000) {  // Redraw every second
        drawMainMenu();
        lastDraw = millis();
    }
}

void MenuManager::drawMainMenu() {
    if (!lcd) return;

    // Clear screen
    lcd->fillScreen(0x0000);  // Black

    // Title
    lcd->setTextColor(0xFFFF);
    lcd->setTextSize(2);
    lcd->setCursor(UIScale::scale(30), UIScale::scale(20));
    lcd->print("BTLogger");

    // Menu items (simple text for now)
    lcd->setTextSize(1);
    lcd->setCursor(UIScale::scale(20), UIScale::scale(60));
    lcd->print("LOG VIEWER");

    lcd->setCursor(UIScale::scale(20), UIScale::scale(80));
    lcd->print("DEVICE MANAGER");

    lcd->setCursor(UIScale::scale(20), UIScale::scale(100));
    lcd->print("FILE BROWSER");

    lcd->setCursor(UIScale::scale(20), UIScale::scale(120));
    lcd->print("SETTINGS");

    lcd->setCursor(UIScale::scale(20), UIScale::scale(140));
    lcd->print("SYSTEM INFO");
}

}  // namespace UI
}  // namespace BTLogger