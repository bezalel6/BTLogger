#include "ScreenManager.hpp"
#include "UIScale.hpp"

namespace BTLogger {
namespace UI {

// Static member definitions
bool ScreenManager::initialized = false;
lgfx::LGFX_Device* ScreenManager::lcd = nullptr;
std::map<String, Screen*> ScreenManager::screens;
std::vector<String> ScreenManager::navigationStack;
Screen* ScreenManager::currentScreen = nullptr;
String ScreenManager::statusText = "Ready";
bool ScreenManager::footerNeedsRedraw = true;

void ScreenManager::initialize(lgfx::LGFX_Device& display) {
    if (initialized) {
        return;
    }

    lcd = &display;
    initialized = true;

    Serial.println("ScreenManager initialized");
}

void ScreenManager::cleanup() {
    if (currentScreen) {
        currentScreen->deactivate();
        currentScreen = nullptr;
    }

    for (auto& pair : screens) {
        pair.second->cleanup();
        delete pair.second;
    }
    screens.clear();
    navigationStack.clear();

    initialized = false;
}

void ScreenManager::registerScreen(Screen* screen) {
    if (!screen || !initialized) {
        return;
    }

    screen->initialize(*lcd);
    screen->setNavigationCallback([](const String& screenName) {
        ScreenManager::navigateTo(screenName);
    });
    screen->setBackCallback([]() {
        ScreenManager::goBack();
    });

    screens[screen->getName()] = screen;
    Serial.printf("Registered screen: %s\n", screen->getName().c_str());
}

void ScreenManager::navigateTo(const String& screenName) {
    Serial.printf("ScreenManager::navigateTo called with: %s\n", screenName.c_str());

    if (!initialized || screens.find(screenName) == screens.end()) {
        Serial.printf("Screen not found: %s\n", screenName.c_str());
        Serial.printf("Available screens: ");
        for (auto& pair : screens) {
            Serial.printf("%s ", pair.first.c_str());
        }
        Serial.println();
        return;
    }

    // Push current screen to stack if we have one
    if (currentScreen) {
        navigationStack.push_back(currentScreen->getName());
        currentScreen->deactivate();
        Serial.printf("Deactivated: %s\n", currentScreen->getName().c_str());
    }

    switchToScreen(screenName);
    footerNeedsRedraw = true;  // Footer needs update for back button indicator
    Serial.printf("Navigated to: %s\n", screenName.c_str());
}

void ScreenManager::goBack() {
    if (!initialized || navigationStack.empty()) {
        return;
    }

    String previousScreen = navigationStack.back();
    navigationStack.pop_back();

    if (currentScreen) {
        currentScreen->deactivate();
    }

    switchToScreen(previousScreen);
    footerNeedsRedraw = true;  // Footer needs update for back button indicator
    Serial.printf("Went back to: %s\n", previousScreen.c_str());
}

void ScreenManager::setStatusText(const String& status) {
    if (statusText != status) {
        statusText = status;
        footerNeedsRedraw = true;
    }
}

void ScreenManager::update() {
    if (!initialized || !currentScreen) {
        return;
    }

    currentScreen->update();

    // Only redraw footer when needed
    if (footerNeedsRedraw) {
        drawStatusFooter();
        footerNeedsRedraw = false;
    }
}

void ScreenManager::handleTouch(int x, int y, bool touched) {
    if (!initialized || !currentScreen) {
        return;
    }

    currentScreen->handleTouch(x, y, touched);
}

String ScreenManager::getCurrentScreenName() {
    if (currentScreen) {
        return currentScreen->getName();
    }
    return "";
}

void ScreenManager::drawStatusFooter() {
    if (!lcd) {
        return;
    }

    int footerY = lcd->height() - Screen::FOOTER_HEIGHT;

    // Draw footer background
    lcd->fillRect(0, footerY, lcd->width(), Screen::FOOTER_HEIGHT, 0x0000);
    lcd->drawFastHLine(0, footerY, lcd->width(), 0x8410);  // Gray line

    // Draw status text
    lcd->setTextColor(0x8410);  // Gray
    lcd->setTextSize(UIScale::scale(1));
    lcd->setCursor(UIScale::scale(5), footerY + UIScale::scale(5));
    lcd->print(statusText.substring(0, 35));  // Limit length to fit screen

    // Draw back button indicator if we can go back
    if (!navigationStack.empty()) {
        lcd->setTextColor(0xFFFF);  // White
        lcd->setCursor(lcd->width() - UIScale::scale(30), footerY + UIScale::scale(5));
        lcd->print("< BACK");
    }
}

void ScreenManager::switchToScreen(const String& screenName) {
    auto it = screens.find(screenName);
    if (it != screens.end()) {
        currentScreen = it->second;
        currentScreen->activate();
        footerNeedsRedraw = true;  // Ensure footer is drawn for new screen
    }
}

}  // namespace UI
}  // namespace BTLogger