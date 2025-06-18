#include "SystemInfoScreen.hpp"
#include "../UIScale.hpp"
#include "../TouchManager.hpp"
#include "../ScreenManager.hpp"

namespace BTLogger {
namespace UI {
namespace Screens {

SystemInfoScreen::SystemInfoScreen() : Screen("SystemInfo"),
                                       backButton(nullptr),
                                       touchCalButton(nullptr),
                                       debugButton(nullptr),
                                       lastTouchState(false),
                                       lastUpdate(0) {
}

SystemInfoScreen::~SystemInfoScreen() {
    cleanup();
}

void SystemInfoScreen::activate() {
    Screen::activate();

    if (!backButton) {
        createButtons();
    }

    ScreenManager::setStatusText("System Info & Diagnostics");
}

void SystemInfoScreen::deactivate() {
    Screen::deactivate();
}

void SystemInfoScreen::update() {
    if (!active) return;

    if (needsRedraw || millis() - lastUpdate > 1000) {
        drawHeader();
        drawSystemInfo();
        needsRedraw = false;
        lastUpdate = millis();
    }

    // Update buttons
    if (backButton) backButton->update();
    if (touchCalButton) touchCalButton->update();
    if (debugButton) debugButton->update();
}

void SystemInfoScreen::handleTouch(int x, int y, bool touched) {
    if (!active) return;

    if (touched || lastTouchState) {
        // Handle button touches
        if (backButton) backButton->handleTouch(x, y, touched);
        if (touchCalButton) touchCalButton->handleTouch(x, y, touched);
        if (debugButton) debugButton->handleTouch(x, y, touched);

        lastTouchState = touched;
    }
}

void SystemInfoScreen::cleanup() {
    delete backButton;
    delete touchCalButton;
    delete debugButton;

    backButton = nullptr;
    touchCalButton = nullptr;
    debugButton = nullptr;
}

void SystemInfoScreen::createButtons() {
    if (!lcd) return;

    int buttonHeight = UIScale::scale(35);
    int buttonY = UIScale::scale(15);
    int margin = UIScale::scale(5);
    int currentX = UIScale::scale(10);

    // Create back button
    backButton = new Widgets::Button(*lcd, currentX, buttonY,
                                     UIScale::scale(BACK_BUTTON_WIDTH), buttonHeight, "BACK");
    backButton->setCallback([this]() {
        Serial.println("Back button pressed in SystemInfo");
        goBack();
    });
    currentX += backButton->getWidth() + margin;

    // Create touch calibration button - calculate available space
    int remainingWidth = lcd->width() - currentX - UIScale::scale(10);         // Reserve 10px right margin
    int touchCalWidth = min(UIScale::scale(90), remainingWidth / 2 - margin);  // Take up to half remaining space

    touchCalButton = new Widgets::Button(*lcd, currentX, buttonY,
                                         touchCalWidth, buttonHeight, "CAL");
    touchCalButton->setCallback([this]() {
        performTouchCalibration();
    });
    currentX += touchCalButton->getWidth() + margin;

    // Create debug button with remaining space
    int debugWidth = lcd->width() - currentX - UIScale::scale(10);  // Use remaining space minus right margin
    debugWidth = max(debugWidth, UIScale::scale(40));               // Minimum 40px

    debugButton = new Widgets::Button(*lcd, currentX, buttonY,
                                      debugWidth, buttonHeight, "DEBUG");
    debugButton->setCallback([this]() {
        showTouchDebugInfo();
    });

    Serial.printf("Header buttons: BACK(%d-%d), CAL(%d-%d), DEBUG(%d-%d), screen width: %d\n",
                  backButton->getX(), backButton->getX() + backButton->getWidth(),
                  touchCalButton->getX(), touchCalButton->getX() + touchCalButton->getWidth(),
                  debugButton->getX(), debugButton->getX() + debugButton->getWidth(),
                  lcd->width());
}

void SystemInfoScreen::drawHeader() {
    if (!lcd) return;

    // Clear header area
    lcd->fillRect(0, 0, lcd->width(), HEADER_HEIGHT, 0x0000);

    // Draw buttons
    if (backButton) backButton->draw();
    if (touchCalButton) touchCalButton->draw();
    if (debugButton) debugButton->draw();

    // Draw separator line
    lcd->drawFastHLine(0, HEADER_HEIGHT - 1, lcd->width(), 0x8410);
}

void SystemInfoScreen::drawSystemInfo() {
    if (!lcd) return;

    // Clear info area
    int infoAreaY = HEADER_HEIGHT;
    int infoAreaHeight = lcd->height() - HEADER_HEIGHT - FOOTER_HEIGHT;
    lcd->fillRect(0, infoAreaY, lcd->width(), infoAreaHeight, 0x0000);

    lcd->setTextColor(0xFFFF);  // White
    lcd->setTextSize(UIScale::scale(1));

    int y = infoAreaY + UIScale::scale(10);
    int lineHeight = UIScale::scale(15);

    // System info
    lcd->setCursor(UIScale::scale(10), y);
    lcd->print("BTLogger System Info");
    y += lineHeight * 2;

    lcd->setCursor(UIScale::scale(10), y);
    lcd->printf("Display: %dx%d", lcd->width(), lcd->height());
    y += lineHeight;

    lcd->setCursor(UIScale::scale(10), y);
    lcd->printf("Free Heap: %d KB", ESP.getFreeHeap() / 1024);
    y += lineHeight;

    lcd->setCursor(UIScale::scale(10), y);
    lcd->printf("Uptime: %lu sec", millis() / 1000);
    y += lineHeight;

    lcd->setCursor(UIScale::scale(10), y);
    lcd->printf("CPU Freq: %d MHz", ESP.getCpuFreqMHz());
    y += lineHeight;

    // Touch info
    y += lineHeight;
    lcd->setCursor(UIScale::scale(10), y);
    lcd->print("Touch Status:");
    y += lineHeight;

    TouchManager::TouchPoint touch = TouchManager::getTouch();
    lcd->setCursor(UIScale::scale(10), y);
    lcd->printf("Position: (%d, %d)", touch.x, touch.y);
    y += lineHeight;

    lcd->setCursor(UIScale::scale(10), y);
    lcd->printf("Pressed: %s", touch.pressed ? "YES" : "NO");
    y += lineHeight;

    lcd->setCursor(UIScale::scale(10), y);
    lcd->printf("Calibrated: %s", TouchManager::needsCalibration() ? "NO" : "YES");
}

void SystemInfoScreen::performTouchCalibration() {
    ScreenManager::setStatusText("Starting touch calibration...");
    TouchManager::resetCalibration();
    ScreenManager::setStatusText("Touch calibration complete");
    markForRedraw();
}

void SystemInfoScreen::showTouchDebugInfo() {
    TouchManager::showTouchDebugInfo();
    ScreenManager::setStatusText("Touch debug info printed to serial");
}

}  // namespace Screens
}  // namespace UI
}  // namespace BTLogger