#pragma once

#include "../Screen.hpp"
#include "../Widgets/Button.hpp"
#include "../../Core/BluetoothManager.hpp"
#include <vector>

namespace BTLogger {
namespace UI {
namespace Screens {

/**
 * Device manager screen for handling BLE device discovery and connections
 */
class DeviceManagerScreen : public Screen {
   public:
    DeviceManagerScreen();
    virtual ~DeviceManagerScreen();

    // Screen interface
    void activate() override;
    void deactivate() override;
    void update() override;
    void handleTouch(int x, int y, bool touched) override;
    void cleanup() override;

    // Device management
    void setBluetoothManager(Core::BluetoothManager* btManager);
    void refreshDeviceList();

   private:
    struct DeviceInfo {
        String name;
        String address;
        bool connected;
        int rssi;
        unsigned long lastSeen;

        DeviceInfo(const String& n, const String& addr, int signal)
            : name(n), address(addr), connected(false), rssi(signal), lastSeen(millis()) {}
    };

    // UI Elements
    Widgets::Button* backButton;
    Widgets::Button* scanButton;
    Widgets::Button* refreshButton;

    // Device list buttons (dynamically created)
    std::vector<Widgets::Button*> deviceButtons;

    // Device data
    std::vector<DeviceInfo> devices;
    Core::BluetoothManager* bluetoothManager;
    int scrollOffset;
    int maxVisibleDevices;
    bool scanning;
    bool lastTouchState;
    unsigned long lastRefresh;

    // Constants
    static const int MAX_DEVICES = 20;
    static const int DEVICE_BUTTON_HEIGHT = 35;
    static const int REFRESH_INTERVAL = 3000;  // 3 seconds

    void createControlButtons();
    void updateDeviceList();
    void drawHeader();
    void drawDeviceList();
    void handleScrolling(int x, int y, bool wasTapped);
    void connectToDevice(const String& address);
    void disconnectFromDevice(const String& address);
    void scrollUp();
    void scrollDown();
    String formatDeviceInfo(const DeviceInfo& device);
    String clipText(const String& text, int maxWidth, int textSize);
};

}  // namespace Screens
}  // namespace UI
}  // namespace BTLogger