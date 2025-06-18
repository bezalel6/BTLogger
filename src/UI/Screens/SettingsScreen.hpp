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
    // Range constants for adjustable settings
    struct AdjustmentRange {
        float start;
        float end;
        float step;

        AdjustmentRange(float s, float e, float st) : start(s), end(e), step(st) {}
    };

    // Constants for adjustment ranges
    static const AdjustmentRange UI_SCALE_RANGE;
    static const AdjustmentRange TEXT_SIZE_RANGE;

    struct SettingItem {
        String name;
        String value;
        std::function<void()> callback;
        bool hasAdjustment;
        std::function<void(bool)> adjustCallback;  // true for +, false for -

        SettingItem(const String& n, const String& v, std::function<void()> cb)
            : name(n), value(v), callback(cb), hasAdjustment(false) {}

        SettingItem(const String& n, const String& v, std::function<void(bool)> adjCb)
            : name(n), value(v), callback(nullptr), hasAdjustment(true), adjustCallback(adjCb) {}
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
    void adjustUIScale(bool increase);
    void adjustLabelTextSize(bool increase);
    void adjustButtonTextSize(bool increase);
    void adjustGeneralTextSize(bool increase);
    void toggleDebugMode();
    void resetSettings();
    void showAbout();

    String getUIScaleText();
    String getLabelTextSizeText();
    String getButtonTextSizeText();
    String getGeneralTextSizeText();
    String getDebugModeText();

    // Helper methods for range-based adjustments
    float adjustValueInRange(float currentValue, const AdjustmentRange& range, bool increase);
    int adjustValueInRange(int currentValue, const AdjustmentRange& range, bool increase);
};

}  // namespace Screens
}  // namespace UI
}  // namespace BTLogger