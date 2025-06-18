#include "SettingsScreen.hpp"
#include "../UIScale.hpp"
#include "../ScreenManager.hpp"

namespace BTLogger {
namespace UI {
namespace Screens {

SettingsScreen::SettingsScreen() : Screen("Settings"), backButton(nullptr), lastTouchState(false) {
}

SettingsScreen::~SettingsScreen() {
    cleanup();
}

void SettingsScreen::activate() {
    Screen::activate();

    if (!backButton) {
        backButton = new Widgets::Button(*lcd, UIScale::scale(10), UIScale::scale(15),
                                         UIScale::scale(BACK_BUTTON_WIDTH), UIScale::scale(35), "BACK");
        backButton->setCallback([this]() {
            Serial.println("Back button pressed in Settings");
            goBack();
        });
    }

    ScreenManager::setStatusText("Settings & Configuration");
}

void SettingsScreen::deactivate() {
    Screen::deactivate();
}

void SettingsScreen::update() {
    if (!active) return;

    if (needsRedraw) {
        drawContent();
        needsRedraw = false;
    }

    if (backButton) backButton->update();
}

void SettingsScreen::handleTouch(int x, int y, bool touched) {
    if (!active) return;

    if (touched || lastTouchState) {
        if (backButton) backButton->handleTouch(x, y, touched);
        lastTouchState = touched;
    }
}

void SettingsScreen::cleanup() {
    delete backButton;
    backButton = nullptr;
}

void SettingsScreen::drawContent() {
    if (!lcd) return;

    lcd->fillScreen(0x0000);

    if (backButton) backButton->draw();

    lcd->setTextColor(0x8410);
    lcd->setTextSize(UIScale::scale(1));
    lcd->setCursor(UIScale::scale(10), UIScale::scale(60));
    lcd->print("Settings");

    lcd->setCursor(UIScale::scale(10), UIScale::scale(80));
    lcd->print("Coming soon...");

    lcd->setCursor(UIScale::scale(10), UIScale::scale(100));
    lcd->print("Will contain configuration options");

    lcd->drawFastHLine(0, HEADER_HEIGHT - 1, lcd->width(), 0x8410);
}

}  // namespace Screens
}  // namespace UI
}  // namespace BTLogger