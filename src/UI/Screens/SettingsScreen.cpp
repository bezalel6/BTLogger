#include "SettingsScreen.hpp"
#include "../UIScale.hpp"
#include "../ScreenManager.hpp"
#include "../TouchManager.hpp"

namespace BTLogger {
namespace UI {
namespace Screens {

SettingsScreen::SettingsScreen() : Screen("Settings"),
                                   backButton(nullptr),
                                   scrollOffset(0),
                                   maxVisibleSettings(0),
                                   lastTouchState(false) {
}

SettingsScreen::~SettingsScreen() {
    cleanup();
}

void SettingsScreen::activate() {
    Screen::activate();

    if (!backButton) {
        createControlButtons();
        createSettings();
    }

    updateSettingsList();
    ScreenManager::setStatusText("Settings & Configuration");
}

void SettingsScreen::deactivate() {
    Screen::deactivate();
}

void SettingsScreen::update() {
    if (!active) return;

    if (needsRedraw) {
        drawSettings();
        needsRedraw = false;
    }

    if (backButton) backButton->update();

    // Update setting buttons
    for (auto button : settingButtons) {
        if (button) button->update();
    }
}

void SettingsScreen::handleTouch(int x, int y, bool touched) {
    if (!active) return;

    bool wasTapped = TouchManager::wasTapped();

    if (wasTapped) {
        handleScrolling(x, y, wasTapped);
    }

    if (touched || lastTouchState) {
        if (backButton) backButton->handleTouch(x, y, touched);

        // Handle setting buttons
        for (auto button : settingButtons) {
            if (button) {
                button->handleTouch(x, y, touched);
            }
        }

        lastTouchState = touched;
    }
}

void SettingsScreen::cleanup() {
    delete backButton;

    for (auto button : settingButtons) {
        delete button;
    }
    settingButtons.clear();

    backButton = nullptr;
    settings.clear();
}

void SettingsScreen::createControlButtons() {
    if (!lcd) return;

    int buttonHeight = UIScale::scale(35);
    int buttonY = UIScale::scale(15);

    backButton = new Widgets::Button(*lcd, UIScale::scale(10), buttonY,
                                     UIScale::scale(BACK_BUTTON_WIDTH), buttonHeight, "BACK");
    backButton->setCallback([this]() {
        Serial.println("Back button pressed in Settings");
        goBack();
    });
}

void SettingsScreen::createSettings() {
    settings.clear();

    // Touch Calibration
    settings.emplace_back("Touch Calibration", "Recalibrate", [this]() {
        calibrateTouch();
    });

    // UI Scale
    settings.emplace_back("UI Scale", getUIScaleText(), [this]() {
        adjustUIScale();
    });

    // Debug Mode
    settings.emplace_back("Debug Output", getDebugModeText(), [this]() {
        toggleDebugMode();
    });

    // Reset Settings
    settings.emplace_back("Reset Settings", "Factory Reset", [this]() {
        resetSettings();
    });

    // About
    settings.emplace_back("About BTLogger", "Version Info", [this]() {
        showAbout();
    });
}

void SettingsScreen::updateSettingsList() {
    // Clear existing buttons
    for (auto button : settingButtons) {
        delete button;
    }
    settingButtons.clear();

    // Calculate layout
    int startY = HEADER_HEIGHT + UIScale::scale(10);
    int buttonHeight = UIScale::scale(SETTING_BUTTON_HEIGHT);
    int buttonSpacing = UIScale::scale(45);
    int buttonWidth = lcd->width() - UIScale::scale(20);

    maxVisibleSettings = (lcd->height() - HEADER_HEIGHT - FOOTER_HEIGHT - UIScale::scale(20)) / buttonSpacing;

    // Create buttons for visible settings
    for (size_t i = 0; i < settings.size(); i++) {
        int visibleIndex = i - scrollOffset;
        if (visibleIndex >= 0 && visibleIndex < maxVisibleSettings) {
            int buttonY = startY + (visibleIndex * buttonSpacing);

            String buttonText = settings[i].name + ": " + settings[i].value;
            auto settingButton = new Widgets::Button(*lcd, UIScale::scale(10), buttonY,
                                                     buttonWidth, buttonHeight, buttonText);

            // Capture setting index for callback
            size_t settingIndex = i;

            settingButton->setCallback([this, settingIndex]() {
                if (settingIndex < settings.size() && settings[settingIndex].callback) {
                    settings[settingIndex].callback();
                    // Refresh settings list after action
                    createSettings();
                    updateSettingsList();
                    markForRedraw();
                }
            });

            // Use different colors for different setting types
            settingButton->setColors(0x001F, 0x051F, 0x8410, 0xFFFF);  // Blue theme

            settingButtons.push_back(settingButton);
        }
    }
}

void SettingsScreen::drawSettings() {
    if (!lcd) return;

    // Clear screen
    lcd->fillScreen(0x0000);

    // Draw header
    if (backButton) backButton->draw();

    // Draw title
    lcd->setTextColor(0x07FF);  // Cyan
    lcd->setTextSize(UIScale::scale(2));
    lcd->setCursor(UIScale::scale(100), UIScale::scale(22));
    lcd->print("SETTINGS");

    lcd->drawFastHLine(0, HEADER_HEIGHT - 1, lcd->width(), 0x8410);

    int infoAreaY = HEADER_HEIGHT;

    if (settings.empty()) {
        lcd->setTextColor(0x8410);
        lcd->setTextSize(UIScale::scale(1));
        lcd->setCursor(UIScale::scale(10), infoAreaY + UIScale::scale(20));
        lcd->print("No settings available");
        return;
    }

    // Draw setting buttons
    for (auto button : settingButtons) {
        if (button) {
            button->draw();
        }
    }

    // Draw scroll indicators
    if (settings.size() > maxVisibleSettings) {
        int indicatorX = lcd->width() - UIScale::scale(10);

        if (scrollOffset > 0) {
            lcd->setTextColor(0xFFFF);
            lcd->setTextSize(1);
            lcd->setCursor(indicatorX, infoAreaY + UIScale::scale(5));
            lcd->print("^");
        }

        if (scrollOffset < (int)settings.size() - maxVisibleSettings) {
            lcd->setTextColor(0xFFFF);
            lcd->setTextSize(1);
            lcd->setCursor(indicatorX, lcd->height() - FOOTER_HEIGHT - UIScale::scale(15));
            lcd->print("v");
        }
    }
}

void SettingsScreen::handleScrolling(int x, int y, bool wasTapped) {
    if (!wasTapped || settings.size() <= maxVisibleSettings) return;

    int infoAreaY = HEADER_HEIGHT;
    int infoAreaHeight = lcd->height() - HEADER_HEIGHT - FOOTER_HEIGHT;

    if (y >= infoAreaY && y < lcd->height() - FOOTER_HEIGHT) {
        if (y < infoAreaY + infoAreaHeight / 2) {
            scrollUp();
        } else {
            scrollDown();
        }
    }
}

void SettingsScreen::scrollUp() {
    if (scrollOffset > 0) {
        scrollOffset--;
        updateSettingsList();
        markForRedraw();
    }
}

void SettingsScreen::scrollDown() {
    int maxScroll = std::max(0, (int)settings.size() - maxVisibleSettings);
    if (scrollOffset < maxScroll) {
        scrollOffset++;
        updateSettingsList();
        markForRedraw();
    }
}

void SettingsScreen::calibrateTouch() {
    ScreenManager::setStatusText("Starting touch calibration...");
    TouchManager::resetCalibration();
    ScreenManager::setStatusText("Touch calibration complete");
}

void SettingsScreen::adjustUIScale() {
    float currentScale = UIScale::getScale();

    // Cycle through scale values: 0.8, 1.0, 1.2, 1.5
    if (currentScale < 0.9f) {
        UIScale::setScale(1.0f);
    } else if (currentScale < 1.1f) {
        UIScale::setScale(1.2f);
    } else if (currentScale < 1.3f) {
        UIScale::setScale(1.5f);
    } else {
        UIScale::setScale(0.8f);
    }

    ScreenManager::setStatusText("UI Scale changed to " + String(UIScale::getScale(), 1) + "x");
}

void SettingsScreen::toggleDebugMode() {
    // This is a placeholder - you'd implement actual debug mode toggle
    static bool debugMode = false;
    debugMode = !debugMode;

    ScreenManager::setStatusText(debugMode ? "Debug mode enabled" : "Debug mode disabled");
}

void SettingsScreen::resetSettings() {
    // Reset UI scale
    UIScale::setScale(1.0f);

    // Clear touch calibration
    TouchManager::resetCalibration();

    ScreenManager::setStatusText("Settings reset to factory defaults");
}

void SettingsScreen::showAbout() {
    ScreenManager::setStatusText("BTLogger v1.0 - ESP32 BLE Log Receiver");
}

String SettingsScreen::getUIScaleText() {
    return String(UIScale::getScale(), 1) + "x";
}

String SettingsScreen::getDebugModeText() {
    // Placeholder - would check actual debug mode state
    static bool debugMode = false;
    return debugMode ? "Enabled" : "Disabled";
}

}  // namespace Screens
}  // namespace UI
}  // namespace BTLogger