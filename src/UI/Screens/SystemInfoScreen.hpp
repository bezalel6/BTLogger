#pragma once

#include "../Screen.hpp"
#include "../Widgets/Button.hpp"

namespace BTLogger {
namespace UI {
namespace Screens {

/**
 * System info screen showing device status and settings
 */
class SystemInfoScreen : public Screen {
   public:
    SystemInfoScreen();
    virtual ~SystemInfoScreen();

    // Screen interface
    void activate() override;
    void deactivate() override;
    void update() override;
    void handleTouch(int x, int y, bool touched) override;
    void cleanup() override;

   private:
    // UI Elements
    Widgets::Button* backButton;
    Widgets::Button* touchCalButton;
    Widgets::Button* debugButton;

    bool lastTouchState;
    unsigned long lastUpdate;

    void createButtons();
    void drawHeader();
    void drawSystemInfo();
    void performTouchCalibration();
    void showTouchDebugInfo();
};

}  // namespace Screens
}  // namespace UI
}  // namespace BTLogger