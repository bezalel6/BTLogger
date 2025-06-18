#pragma once

#include "../Screen.hpp"
#include "../Widgets/Button.hpp"

namespace BTLogger {
namespace UI {
namespace Screens {

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
    Widgets::Button* backButton;
    bool lastTouchState;
    void drawContent();
};

}  // namespace Screens
}  // namespace UI
}  // namespace BTLogger