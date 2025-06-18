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

    // Start scanning automatically when entering screen
    if (bluetoothManager) {
        bluetoothManager->startScanning();
        scanning = true;
        scanButton->setText("STOP");
    }

    refreshDeviceList();
    ScreenManager::setStatusText("Device Manager - BLE Devices");
}

void DeviceManagerScreen::deactivate() {
    Screen::deactivate();
}

void DeviceManagerScreen::update() {
    if (!active) return;

    // Auto-refresh device list periodically
    if (millis() - lastRefresh > REFRESH_INTERVAL) {
        refreshDeviceList();
        lastRefresh = millis();
    }

    if (needsRedraw) {
        drawHeader();
        drawDeviceList();
        needsRedraw = false;
    }

    // Update buttons
    if (backButton) backButton->update();
    if (scanButton) scanButton->update();
    if (refreshButton) refreshButton->update();

    // Update device buttons
    for (auto button : deviceButtons) {
        if (button) button->update();
    }
}

void DeviceManagerScreen::handleTouch(int x, int y, bool touched) {
    if (!active) return;

    bool wasTapped = TouchManager::wasTapped();

    // Handle scrolling for device list
    if (wasTapped) {
        handleScrolling(x, y, wasTapped);
    }

    if (touched || lastTouchState) {
        // Handle control buttons
        if (backButton) backButton->handleTouch(x, y, touched);
        if (scanButton) scanButton->handleTouch(x, y, touched);
        if (refreshButton) refreshButton->handleTouch(x, y, touched);

        // Handle device buttons
        for (auto button : deviceButtons) {
            if (button) {
                button->handleTouch(x, y, touched);
            }
        }

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
    if (!bluetoothManager) {
        ScreenManager::setStatusText("Bluetooth not available");
        return;
    }

    // Get discovered devices from BluetoothManager
    // This is a placeholder - you'd need to implement getDiscoveredDevices() in BluetoothManager
    devices.clear();

    // For now, add some example devices to demonstrate functionality
    devices.emplace_back("WeatherStation_v2.1", "AA:BB:CC:DD:EE:FF", -45);
    devices.emplace_back("ESP32_Logger", "11:22:33:44:55:66", -67);
    devices.emplace_back("BTLogger_Test", "99:88:77:66:55:44", -32);

    // Update device connection status
    for (auto& device : devices) {
        device.connected = bluetoothManager->isDeviceConnected(device.address);
    }

    updateDeviceList();
    markForRedraw();

    ScreenManager::setStatusText(String("Found ") + devices.size() + " devices");
}

void DeviceManagerScreen::createControlButtons() {
    if (!lcd) return;

    int buttonHeight = UIScale::scale(35);
    int buttonY = UIScale::scale(15);

    backButton = new Widgets::Button(*lcd, UIScale::scale(10), buttonY,
                                     UIScale::scale(BACK_BUTTON_WIDTH), buttonHeight, "BACK");
    backButton->setCallback([this]() {
        Serial.println("Back button pressed in DeviceManager");
        // Stop scanning when leaving
        if (bluetoothManager && scanning) {
            bluetoothManager->stopScanning();
            scanning = false;
        }
        goBack();
    });

    scanButton = new Widgets::Button(*lcd, UIScale::scale(100), buttonY,
                                     UIScale::scale(50), buttonHeight, scanning ? "STOP" : "SCAN");
    scanButton->setCallback([this]() {
        if (bluetoothManager) {
            scanning = !scanning;
            if (scanning) {
                bluetoothManager->startScanning();
                scanButton->setText("STOP");
                ScreenManager::setStatusText("Scanning for devices...");
            } else {
                bluetoothManager->stopScanning();
                scanButton->setText("SCAN");
                ScreenManager::setStatusText("Scan stopped");
            }
        }
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

    if (devices.empty()) {
        lcd->setTextColor(0x8410);  // Gray
        lcd->setTextSize(UIScale::scale(1));
        lcd->setCursor(UIScale::scale(10), infoAreaY + UIScale::scale(20));
        if (scanning) {
            lcd->print("Scanning for devices...");
        } else {
            lcd->print("No devices found");
            lcd->setCursor(UIScale::scale(10), infoAreaY + UIScale::scale(40));
            lcd->print("Press SCAN to search");
        }
        return;
    }

    // Draw all device buttons
    for (auto button : deviceButtons) {
        if (button) {
            button->draw();
        }
    }

    // Draw scroll indicators if needed
    if (devices.size() > maxVisibleDevices) {
        int indicatorX = lcd->width() - UIScale::scale(10);

        if (scrollOffset > 0) {
            lcd->setTextColor(0xFFFF);
            lcd->setTextSize(1);
            lcd->setCursor(indicatorX, infoAreaY + UIScale::scale(5));
            lcd->print("^");
        }

        if (scrollOffset < (int)devices.size() - maxVisibleDevices) {
            lcd->setTextColor(0xFFFF);
            lcd->setTextSize(1);
            lcd->setCursor(indicatorX, lcd->height() - FOOTER_HEIGHT - UIScale::scale(15));
            lcd->print("v");
        }
    }
}

void DeviceManagerScreen::handleScrolling(int x, int y, bool wasTapped) {
    if (!wasTapped || devices.size() <= maxVisibleDevices) return;

    int infoAreaY = HEADER_HEIGHT;
    int infoAreaHeight = lcd->height() - HEADER_HEIGHT - FOOTER_HEIGHT;

    // Check for scroll in device list area
    if (y >= infoAreaY && y < lcd->height() - FOOTER_HEIGHT) {
        // Top half scrolls up, bottom half scrolls down
        if (y < infoAreaY + infoAreaHeight / 2) {
            scrollUp();
        } else {
            scrollDown();
        }
    }
}

void DeviceManagerScreen::updateDeviceList() {
    // Clear existing device buttons
    for (auto button : deviceButtons) {
        delete button;
    }
    deviceButtons.clear();

    // Create buttons for each device
    int startY = HEADER_HEIGHT + UIScale::scale(10);
    int buttonHeight = UIScale::scale(DEVICE_BUTTON_HEIGHT);
    int buttonSpacing = UIScale::scale(40);
    int buttonWidth = lcd->width() - UIScale::scale(20);

    maxVisibleDevices = (lcd->height() - HEADER_HEIGHT - FOOTER_HEIGHT - UIScale::scale(20)) / buttonSpacing;

    for (size_t i = 0; i < devices.size() && i < MAX_DEVICES; i++) {
        int visibleIndex = i - scrollOffset;
        if (visibleIndex >= 0 && visibleIndex < maxVisibleDevices) {
            int buttonY = startY + (visibleIndex * buttonSpacing);

            String buttonText = formatDeviceInfo(devices[i]);
            auto deviceButton = new Widgets::Button(*lcd, UIScale::scale(10), buttonY,
                                                    buttonWidth, buttonHeight, buttonText);

            // Capture device address for callback
            String address = devices[i].address;
            bool connected = devices[i].connected;

            deviceButton->setCallback([this, address, connected]() {
                if (connected) {
                    disconnectFromDevice(address);
                } else {
                    connectToDevice(address);
                }
            });

            // Color based on connection status
            if (devices[i].connected) {
                deviceButton->setColors(0x07E0, 0x07E8, 0x8410, 0x0000);  // Green when connected
            } else {
                deviceButton->setColors(0x001F, 0x051F, 0x8410, 0xFFFF);  // Blue when disconnected
            }

            deviceButtons.push_back(deviceButton);
        }
    }
}

String DeviceManagerScreen::formatDeviceInfo(const DeviceInfo& device) {
    String status = device.connected ? "CONN" : "DISC";
    String signalStr = String(device.rssi) + "dBm";
    return device.name + " [" + status + "] " + signalStr;
}

void DeviceManagerScreen::connectToDevice(const String& address) {
    if (bluetoothManager) {
        Serial.printf("Connecting to device: %s\n", address.c_str());
        bluetoothManager->connectToDevice(address);
        ScreenManager::setStatusText("Connecting to device...");
    }
}

void DeviceManagerScreen::disconnectFromDevice(const String& address) {
    if (bluetoothManager) {
        Serial.printf("Disconnecting from device: %s\n", address.c_str());
        bluetoothManager->disconnectFromDevice(address);
        ScreenManager::setStatusText("Disconnecting from device...");
    }
}

void DeviceManagerScreen::scrollUp() {
    if (scrollOffset > 0) {
        scrollOffset--;
        updateDeviceList();
        markForRedraw();
    }
}

void DeviceManagerScreen::scrollDown() {
    int maxScroll = std::max(0, (int)devices.size() - maxVisibleDevices);
    if (scrollOffset < maxScroll) {
        scrollOffset++;
        updateDeviceList();
        markForRedraw();
    }
}

}  // namespace Screens
}  // namespace UI
}  // namespace BTLogger