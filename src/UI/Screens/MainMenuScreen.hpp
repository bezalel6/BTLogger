#pragma once

#include "../Screen.hpp"
#include "../Widgets/Button.hpp"

namespace BTLogger {
namespace UI {
namespace Screens {

/**
 * Main menu screen with navigation buttons
 */
class MainMenuScreen : public Screen {
   public:
    MainMenuScreen();
    virtual ~MainMenuScreen();

    // Screen interface
    void activate() override;
    void deactivate() override;
    void update() override;
    void handleTouch(int x, int y, bool touched) override;
    void cleanup() override;

   private:
    // Menu buttons
    Widgets::Button* logViewerButton;
    Widgets::Button* deviceManagerButton;
    Widgets::Button* fileBrowserButton;
    Widgets::Button* settingsButton;
    Widgets::Button* systemInfoButton;

    // Scrolling
    int scrollOffset;
    int maxScrollOffset;
    static const int BUTTON_COUNT = 5;
    static const int VISIBLE_BUTTONS = 4;

    bool lastTouchState;

    void createButtons();
    void drawMenu();
    void handleScrolling(int x, int y, bool wasTapped);
    void updateButtonPositions();
    void scrollUp();
    void scrollDown();
};

}  // namespace Screens
}  // namespace UI
}  // namespace BTLogger