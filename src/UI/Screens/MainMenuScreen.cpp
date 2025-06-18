#include "MainMenuScreen.hpp"
#include "../UIScale.hpp"
#include "../TouchManager.hpp"
#include "../ScreenManager.hpp"
#include <algorithm>

namespace BTLogger {
namespace UI {
namespace Screens {

MainMenuScreen::MainMenuScreen() : Screen("MainMenu"),
                                   logViewerButton(nullptr),
                                   deviceManagerButton(nullptr),
                                   fileBrowserButton(nullptr),
                                   settingsButton(nullptr),
                                   systemInfoButton(nullptr),
                                   scrollOffset(0),
                                   maxScrollOffset(0),
                                   lastTouchState(false) {
}

MainMenuScreen::~MainMenuScreen() {
    cleanup();
}

void MainMenuScreen::activate() {
    Screen::activate();
    if (!logViewerButton) {
        createButtons();
    }
    ScreenManager::setStatusText("Touch buttons to navigate");
}

void MainMenuScreen::deactivate() {
    Screen::deactivate();
}

void MainMenuScreen::update() {
    if (!active) return;

    if (needsRedraw) {
        drawMenu();
        needsRedraw = false;
    }

    // Update all buttons
    if (logViewerButton) logViewerButton->update();
    if (deviceManagerButton) deviceManagerButton->update();
    if (fileBrowserButton) fileBrowserButton->update();
    if (settingsButton) settingsButton->update();
    if (systemInfoButton) systemInfoButton->update();
}

void MainMenuScreen::handleTouch(int x, int y, bool touched) {
    if (!active) return;

    bool wasTapped = TouchManager::wasTapped();

    // Handle scrolling first
    if (wasTapped) {
        handleScrolling(x, y, wasTapped);
    }

    if (touched || lastTouchState) {
        // Handle button touches
        if (logViewerButton) logViewerButton->handleTouch(x, y, touched);
        if (deviceManagerButton) deviceManagerButton->handleTouch(x, y, touched);
        if (fileBrowserButton) fileBrowserButton->handleTouch(x, y, touched);
        if (settingsButton) settingsButton->handleTouch(x, y, touched);
        if (systemInfoButton) systemInfoButton->handleTouch(x, y, touched);

        lastTouchState = touched;
    }
}

void MainMenuScreen::cleanup() {
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

void MainMenuScreen::createButtons() {
    if (!lcd) return;

    int buttonWidth = UIScale::scale(200);
    int buttonHeight = UIScale::scale(35);
    int buttonX = (lcd->width() - buttonWidth) / 2;
    int startY = UIScale::scale(70);
    int buttonSpacing = UIScale::scale(45);

    logViewerButton = new Widgets::Button(*lcd, buttonX, startY, buttonWidth, buttonHeight, "LOG VIEWER");
    logViewerButton->setCallback([this]() {
        Serial.println("Log Viewer button pressed!");
        ScreenManager::setStatusText("Opening Log Viewer...");
        this->navigateTo("LogViewer");
    });

    deviceManagerButton = new Widgets::Button(*lcd, buttonX, startY + buttonSpacing, buttonWidth, buttonHeight, "DEVICE MANAGER");
    deviceManagerButton->setCallback([this]() {
        Serial.println("Device Manager button pressed!");
        ScreenManager::setStatusText("Opening Device Manager...");
        this->navigateTo("DeviceManager");
    });

    fileBrowserButton = new Widgets::Button(*lcd, buttonX, startY + buttonSpacing * 2, buttonWidth, buttonHeight, "FILE BROWSER");
    fileBrowserButton->setCallback([this]() {
        Serial.println("File Browser button pressed!");
        ScreenManager::setStatusText("Opening File Browser...");
        this->navigateTo("FileBrowser");
    });

    settingsButton = new Widgets::Button(*lcd, buttonX, startY + buttonSpacing * 3, buttonWidth, buttonHeight, "SETTINGS");
    settingsButton->setCallback([this]() {
        Serial.println("Settings button pressed!");
        ScreenManager::setStatusText("Opening Settings...");
        this->navigateTo("Settings");
    });

    systemInfoButton = new Widgets::Button(*lcd, buttonX, startY + buttonSpacing * 4, buttonWidth, buttonHeight, "SYSTEM INFO");
    systemInfoButton->setCallback([this]() {
        Serial.println("System Info button pressed!");
        ScreenManager::setStatusText("Opening System Info...");
        this->navigateTo("SystemInfo");
    });

    // Calculate scroll limits
    maxScrollOffset = std::max(0, BUTTON_COUNT - VISIBLE_BUTTONS);

    Serial.printf("MainMenu: %d buttons, %d visible, maxScroll=%d\n", BUTTON_COUNT, VISIBLE_BUTTONS, maxScrollOffset);
}

void MainMenuScreen::drawMenu() {
    if (!lcd) return;

    // Clear screen
    lcd->fillScreen(0x0000);

    // Title
    lcd->setTextColor(0x07FF);  // Cyan
    lcd->setTextSize(UIScale::scale(3));
    int titleY = UIScale::scale(15);
    lcd->setCursor(UIScale::scale(30), titleY);
    lcd->print("BTLogger");

    // Title line
    int lineY = titleY + UIScale::scale(30);
    lcd->drawFastHLine(UIScale::scale(10), lineY, lcd->width() - UIScale::scale(20), 0x07FF);

    // Update button positions and draw visible ones
    updateButtonPositions();

    // Draw scroll indicators
    if (maxScrollOffset > 0) {
        int indicatorX = lcd->width() - UIScale::scale(15);

        if (scrollOffset > 0) {
            lcd->setTextColor(0xFFFF);
            lcd->setTextSize(1);
            lcd->setCursor(indicatorX, lineY + UIScale::scale(10));
            lcd->print("^");
        }

        if (scrollOffset < maxScrollOffset) {
            lcd->setTextColor(0xFFFF);
            lcd->setTextSize(1);
            lcd->setCursor(indicatorX, lcd->height() - FOOTER_HEIGHT - UIScale::scale(15));
            lcd->print("v");
        }
    }
}

void MainMenuScreen::handleScrolling(int x, int y, bool wasTapped) {
    if (!wasTapped || maxScrollOffset == 0) return;

    int lineY = UIScale::scale(15) + UIScale::scale(30);
    int scrollZoneHeight = UIScale::scale(30);

    // Check for scroll in right edge
    if (x > lcd->width() - UIScale::scale(30)) {
        // Touch in top scroll zone
        if (y >= lineY && y < lineY + scrollZoneHeight) {
            scrollUp();
            return;
        }

        // Touch in bottom scroll zone
        if (y >= lcd->height() - FOOTER_HEIGHT - scrollZoneHeight && y < lcd->height() - FOOTER_HEIGHT) {
            scrollDown();
            return;
        }
    }
}

void MainMenuScreen::updateButtonPositions() {
    if (!lcd) return;

    int startY = UIScale::scale(70);
    int buttonSpacing = UIScale::scale(45);
    int buttonX = (lcd->width() - UIScale::scale(200)) / 2;

    Widgets::Button* buttons[] = {logViewerButton, deviceManagerButton, fileBrowserButton, settingsButton, systemInfoButton};

    for (int i = 0; i < BUTTON_COUNT; i++) {
        if (buttons[i]) {
            int newY = startY + (i * buttonSpacing) - (scrollOffset * buttonSpacing);
            buttons[i]->setPosition(buttonX, newY);

            // Only draw if visible
            int lineY = UIScale::scale(15) + UIScale::scale(30);
            if (newY >= lineY + UIScale::scale(10) && newY < lcd->height() - FOOTER_HEIGHT) {
                buttons[i]->draw();
            }
        }
    }
}

void MainMenuScreen::scrollUp() {
    if (scrollOffset > 0) {
        scrollOffset--;
        markForRedraw();
        ScreenManager::setStatusText("Scrolled up");
    }
}

void MainMenuScreen::scrollDown() {
    if (scrollOffset < maxScrollOffset) {
        scrollOffset++;
        markForRedraw();
        ScreenManager::setStatusText("Scrolled down");
    }
}

}  // namespace Screens
}  // namespace UI
}  // namespace BTLogger