#include "SettingsScreen.hpp"
#include "../UIScale.hpp"
#include "../ScreenManager.hpp"
#include "../TouchManager.hpp"

namespace BTLogger {
namespace UI {
namespace Screens {

// Define adjustment ranges
const SettingsScreen::AdjustmentRange SettingsScreen::UI_SCALE_RANGE(0.8f, 1.5f, 0.1f);
const SettingsScreen::AdjustmentRange SettingsScreen::TEXT_SIZE_RANGE(0.8f, 2.0f, 0.1f);

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

    // Use full width for back button since it's the only header button
    backButton = new Widgets::Button(*lcd, 0, buttonY,
                                     lcd->width(), buttonHeight, "BACK");
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

    // UI Scale (with adjustment buttons)
    settings.emplace_back("UI Scale", getUIScaleText(), [this](bool increase) {
        adjustUIScale(increase);
    });

    // Text Size Settings (with adjustment buttons)
    settings.emplace_back("Label Text Size", getLabelTextSizeText(), [this](bool increase) {
        adjustLabelTextSize(increase);
    });

    settings.emplace_back("Button Text Size", getButtonTextSizeText(), [this](bool increase) {
        adjustButtonTextSize(increase);
    });

    settings.emplace_back("General Text Size", getGeneralTextSizeText(), [this](bool increase) {
        adjustGeneralTextSize(increase);
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
    int totalWidth = lcd->width() - UIScale::scale(20);

    maxVisibleSettings = (lcd->height() - HEADER_HEIGHT - FOOTER_HEIGHT - UIScale::scale(20)) / buttonSpacing;

    // Create buttons for visible settings
    for (size_t i = 0; i < settings.size(); i++) {
        int visibleIndex = i - scrollOffset;
        if (visibleIndex >= 0 && visibleIndex < maxVisibleSettings) {
            int buttonY = startY + (visibleIndex * buttonSpacing);
            size_t settingIndex = i;

            if (settings[i].hasAdjustment) {
                // Create label + buttons layout for adjustable settings
                int labelWidth = totalWidth * 0.5f;
                int buttonWidth = totalWidth * 0.2f;
                int buttonGap = UIScale::scale(5);

                // Setting label
                String labelText = settings[i].name + ": " + settings[i].value;
                auto labelButton = new Widgets::Button(*lcd, UIScale::scale(10), buttonY,
                                                       labelWidth, buttonHeight, labelText);
                labelButton->setColors(0x0000, 0x0000, 0x8410, 0xFFFF);  // Read-only appearance
                settingButtons.push_back(labelButton);

                // Minus button
                auto minusButton = new Widgets::Button(*lcd, UIScale::scale(10) + labelWidth + buttonGap, buttonY,
                                                       buttonWidth, buttonHeight, "-");
                minusButton->setCallback([this, settingIndex]() {
                    if (settingIndex < settings.size() && settings[settingIndex].adjustCallback) {
                        settings[settingIndex].adjustCallback(false);  // false = decrease
                        createSettings();
                        updateSettingsList();
                        markForRedraw();
                    }
                });
                minusButton->setColors(0x8000, 0xA000, 0x8410, 0xFFFF);  // Red theme for minus
                settingButtons.push_back(minusButton);

                // Plus button
                auto plusButton = new Widgets::Button(*lcd, UIScale::scale(10) + labelWidth + buttonGap + buttonWidth + buttonGap, buttonY,
                                                      buttonWidth, buttonHeight, "+");
                plusButton->setCallback([this, settingIndex]() {
                    if (settingIndex < settings.size() && settings[settingIndex].adjustCallback) {
                        settings[settingIndex].adjustCallback(true);  // true = increase
                        createSettings();
                        updateSettingsList();
                        markForRedraw();
                    }
                });
                plusButton->setColors(0x03E0, 0x07E0, 0x8410, 0xFFFF);  // Green theme for plus
                settingButtons.push_back(plusButton);
            } else {
                // Single button for non-adjustable settings
                String buttonText = settings[i].name + ": " + settings[i].value;
                auto settingButton = new Widgets::Button(*lcd, UIScale::scale(10), buttonY,
                                                         totalWidth, buttonHeight, buttonText);

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
}

void SettingsScreen::drawSettings() {
    if (!lcd) return;

    // Clear screen
    lcd->fillScreen(0x0000);

    // Draw header - just buttons, no title
    if (backButton) backButton->draw();

    lcd->drawFastHLine(0, HEADER_HEIGHT - 1, lcd->width(), 0x8410);

    int infoAreaY = HEADER_HEIGHT;

    if (settings.empty()) {
        lcd->setTextColor(0x8410);
        lcd->setTextSize(UIScale::getGeneralTextSize());
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
            lcd->setTextSize(UIScale::getGeneralTextSize());
            lcd->setCursor(indicatorX, infoAreaY + UIScale::scale(5));
            lcd->print("^");
        }

        if (scrollOffset < (int)settings.size() - maxVisibleSettings) {
            lcd->setTextColor(0xFFFF);
            lcd->setTextSize(UIScale::getGeneralTextSize());
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

// Helper methods for range-based adjustments
float SettingsScreen::adjustValueInRange(float currentValue, const AdjustmentRange& range, bool increase) {
    if (increase) {
        float newValue = currentValue + range.step;
        return (newValue <= range.end) ? newValue : range.end;
    } else {
        float newValue = currentValue - range.step;
        return (newValue >= range.start) ? newValue : range.start;
    }
}

int SettingsScreen::adjustValueInRange(int currentValue, const AdjustmentRange& range, bool increase) {
    if (increase) {
        int newValue = currentValue + (int)range.step;
        return (newValue <= (int)range.end) ? newValue : (int)range.end;
    } else {
        int newValue = currentValue - (int)range.step;
        return (newValue >= (int)range.start) ? newValue : (int)range.start;
    }
}

void SettingsScreen::adjustUIScale(bool increase) {
    float currentScale = UIScale::getScale();
    float newScale = adjustValueInRange(currentScale, UI_SCALE_RANGE, increase);

    UIScale::setScale(newScale);
    ScreenManager::setStatusText("UI Scale changed to " + String(newScale, 1) + "x");
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

    // Reset text sizes to defaults
    UIScale::setLabelTextSize(1);
    UIScale::setButtonTextSize(2);
    UIScale::setGeneralTextSize(1);

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

String SettingsScreen::getLabelTextSizeText() {
    return "Size " + String(UIScale::getLabelTextSize());
}

String SettingsScreen::getButtonTextSizeText() {
    return "Size " + String(UIScale::getButtonTextSize());
}

String SettingsScreen::getGeneralTextSizeText() {
    return "Size " + String(UIScale::getGeneralTextSize());
}

void SettingsScreen::adjustLabelTextSize(bool increase) {
    int currentSize = UIScale::getLabelTextSize();
    int newSize = adjustValueInRange(currentSize, TEXT_SIZE_RANGE, increase);

    UIScale::setLabelTextSize(newSize);
    ScreenManager::setStatusText("Label text size: " + String(newSize));
}

void SettingsScreen::adjustButtonTextSize(bool increase) {
    int currentSize = UIScale::getButtonTextSize();
    int newSize = adjustValueInRange(currentSize, TEXT_SIZE_RANGE, increase);

    UIScale::setButtonTextSize(newSize);
    ScreenManager::setStatusText("Button text size: " + String(newSize));
}

void SettingsScreen::adjustGeneralTextSize(bool increase) {
    int currentSize = UIScale::getGeneralTextSize();
    int newSize = adjustValueInRange(currentSize, TEXT_SIZE_RANGE, increase);

    UIScale::setGeneralTextSize(newSize);
    ScreenManager::setStatusText("General text size: " + String(newSize));
}

}  // namespace Screens
}  // namespace UI
}  // namespace BTLogger