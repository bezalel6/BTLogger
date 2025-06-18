#include "DeviceManagerScreen.hpp"
#include "../UIScale.hpp"
#include "../TouchManager.hpp"
#include "../ScreenManager.hpp"

namespace BTLogger {
namespace UI {
namespace Screens {

DeviceManagerScreen::DeviceManagerScreen() : Screen("DeviceManager"),
                                             backButton(nullptr),
                                             scanButton(nullptr),
                                             refreshButton(nullptr),
                                             bluetoothManager(nullptr),
                                             scrollOffset(0),
                                             maxVisibleDevices(0),
                                             scanning(false),
                                             lastTouchState(false),
                                             lastRefresh(0) {
}

DeviceManagerScreen::~DeviceManagerScreen() {
    cleanup();
}

void DeviceManagerScreen::activate() {
    Screen::activate();
    createControlButtons();
    ScreenManager::setStatusText("Device Manager - BLE Devices");
}

void DeviceManagerScreen::deactivate() {
    Screen::deactivate();
}

void DeviceManagerScreen::update() {
    if (!active) return;

    if (needsRedraw) {
        drawHeader();
        drawDeviceList();
        needsRedraw = false;
    }

    // Update buttons
    if (backButton) backButton->update();
    if (scanButton) scanButton->update();
    if (refreshButton) refreshButton->update();
}

void DeviceManagerScreen::handleTouch(int x, int y, bool touched) {
    if (!active) return;

    if (touched || lastTouchState) {
        if (backButton) backButton->handleTouch(x, y, touched);
        if (scanButton) scanButton->handleTouch(x, y, touched);
        if (refreshButton) refreshButton->handleTouch(x, y, touched);
        lastTouchState = touched;
    }
}

void DeviceManagerScreen::cleanup() {
    delete backButton;
    delete scanButton;
    delete refreshButton;

    backButton = nullptr;
    scanButton = nullptr;
    refreshButton = nullptr;
}

void DeviceManagerScreen::setBluetoothManager(Core::BluetoothManager* btManager) {
    bluetoothManager = btManager;
}

void DeviceManagerScreen::refreshDeviceList() {
    // Placeholder - would query bluetooth manager for devices
    ScreenManager::setStatusText("Device list refreshed");
}

void DeviceManagerScreen::createControlButtons() {
    if (!lcd) return;

    int buttonHeight = UIScale::scale(35);
    int buttonY = UIScale::scale(15);

    backButton = new Widgets::Button(*lcd, UIScale::scale(10), buttonY,
                                     UIScale::scale(BACK_BUTTON_WIDTH), buttonHeight, "BACK");
    backButton->setCallback([this]() {
        Serial.println("Back button pressed in DeviceManager");
        goBack();
    });

    scanButton = new Widgets::Button(*lcd, UIScale::scale(100), buttonY,
                                     UIScale::scale(50), buttonHeight, "SCAN");
    scanButton->setCallback([this]() {
        scanning = !scanning;
        scanButton->setText(scanning ? "STOP" : "SCAN");
        ScreenManager::setStatusText(scanning ? "Scanning..." : "Scan stopped");
    });

    refreshButton = new Widgets::Button(*lcd, UIScale::scale(160), buttonY,
                                        UIScale::scale(70), buttonHeight, "REFRESH");
    refreshButton->setCallback([this]() {
        refreshDeviceList();
    });
}

void DeviceManagerScreen::drawHeader() {
    if (!lcd) return;

    lcd->fillRect(0, 0, lcd->width(), HEADER_HEIGHT, 0x0000);

    if (backButton) backButton->draw();
    if (scanButton) scanButton->draw();
    if (refreshButton) refreshButton->draw();

    lcd->drawFastHLine(0, HEADER_HEIGHT - 1, lcd->width(), 0x8410);
}

void DeviceManagerScreen::drawDeviceList() {
    if (!lcd) return;

    int infoAreaY = HEADER_HEIGHT;
    int infoAreaHeight = lcd->height() - HEADER_HEIGHT - FOOTER_HEIGHT;
    lcd->fillRect(0, infoAreaY, lcd->width(), infoAreaHeight, 0x0000);

    lcd->setTextColor(0x8410);  // Gray
    lcd->setTextSize(UIScale::scale(1));
    lcd->setCursor(UIScale::scale(10), infoAreaY + UIScale::scale(20));
    lcd->print("Device Manager");

    lcd->setCursor(UIScale::scale(10), infoAreaY + UIScale::scale(40));
    lcd->print("Coming soon...");

    lcd->setCursor(UIScale::scale(10), infoAreaY + UIScale::scale(60));
    lcd->print("Will show discovered BLE devices");
}

void DeviceManagerScreen::handleScrolling(int x, int y, bool wasTapped) {
    // Placeholder
}

void DeviceManagerScreen::connectToDevice(const String& address) {
    // Placeholder
}

void DeviceManagerScreen::disconnectFromDevice(const String& address) {
    // Placeholder
}

void DeviceManagerScreen::scrollUp() {
    // Placeholder
}

void DeviceManagerScreen::scrollDown() {
    // Placeholder
}

void DeviceManagerScreen::updateDeviceList() {
    // Placeholder
}

String DeviceManagerScreen::formatDeviceInfo(const DeviceInfo& device) {
    return device.name + " (" + device.address + ")";
}

}  // namespace Screens
}  // namespace UI
}  // namespace BTLogger