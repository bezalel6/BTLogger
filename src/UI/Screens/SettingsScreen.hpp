#pragma once

#include "../Screen.hpp"
#include "../Widgets/Button.hpp"
#include <vector>

namespace BTLogger {
namespace UI {
namespace Screens {

/**
 * Settings screen for system configuration
 */
class SettingsScreen : public Screen {
   public:
    SettingsScreen();
    virtual ~SettingsScreen();

    void activate() override;
    void deactivate() override;
    void update() override;
    void handleTouch(int x, int y, bool touched) override;
    void cleanup() override;

   private:
    struct SettingItem {
        String name;
        String value;
        std::function<void()> callback;

        SettingItem(const String& n, const String& v, std::function<void()> cb)
            : name(n), value(v), callback(cb) {}
    };

    // UI Elements
    Widgets::Button* backButton;

    // Setting buttons (dynamically created)
    std::vector<Widgets::Button*> settingButtons;
    std::vector<SettingItem> settings;

    int scrollOffset;
    int maxVisibleSettings;
    bool lastTouchState;

    // Constants
    static const int SETTING_BUTTON_HEIGHT = 35;

    void createControlButtons();
    void createSettings();
    void updateSettingsList();
    void drawSettings();
    void handleScrolling(int x, int y, bool wasTapped);
    void scrollUp();
    void scrollDown();

    // Setting actions
    void calibrateTouch();
    void adjustUIScale();
    void adjustLabelTextSize();
    void adjustButtonTextSize();
    void adjustGeneralTextSize();
    void toggleDebugMode();
    void resetSettings();
    void showAbout();

    String getUIScaleText();
    String getLabelTextSizeText();
    String getButtonTextSizeText();
    String getGeneralTextSizeText();
    String getDebugModeText();
};

}  // namespace Screens
}  // namespace UI
}  // namespace BTLogger