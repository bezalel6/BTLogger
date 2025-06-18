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

    for (auto button : deviceButtons) {
        delete button;
    }
    deviceButtons.clear();

    backButton = nullptr;
    scanButton = nullptr;
    refreshButton = nullptr;
    devices.clear();
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
    auto connectedDevices = bluetoothManager->getConnectedDeviceNames();
    for (auto& device : devices) {
        device.connected = false;
        for (const auto& connectedName : connectedDevices) {
            if (device.name == connectedName) {
                device.connected = true;
                break;
            }
        }
    }

    updateDeviceList();
    markForRedraw();

    ScreenManager::setStatusText(String("Found ") + devices.size() + " devices");
}

void DeviceManagerScreen::createControlButtons() {
    if (!lcd) return;

    int buttonHeight = UIScale::scale(35);
    int buttonY = UIScale::scale(15);

    // Calculate button widths to divide the full screen width
    int totalWidth = lcd->width();
    int buttonCount = 3;  // BACK, SCAN, REFRESH
    int buttonWidth = totalWidth / buttonCount;

    int currentX = 0;

    // Create back button - takes 1/3 of width
    backButton = new Widgets::Button(*lcd, currentX, buttonY,
                                     buttonWidth, buttonHeight, "BACK");
    backButton->setCallback([this]() {
        Serial.println("Back button pressed in DeviceManager");
        // Stop scanning when leaving
        if (bluetoothManager && scanning) {
            bluetoothManager->stopScanning();
            scanning = false;
        }
        goBack();
    });
    currentX += buttonWidth;

    // Create scan button - takes 1/3 of width
    scanButton = new Widgets::Button(*lcd, currentX, buttonY,
                                     buttonWidth, buttonHeight, scanning ? "STOP" : "SCAN");
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
    currentX += buttonWidth;

    // Create refresh button - takes remaining width (handles any rounding)
    int remainingWidth = totalWidth - currentX;
    refreshButton = new Widgets::Button(*lcd, currentX, buttonY,
                                        remainingWidth, buttonHeight, "REFRESH");
    refreshButton->setCallback([this]() {
        refreshDeviceList();
    });

    Serial.printf("DeviceManager buttons: BACK(%d-%d), SCAN(%d-%d), REFRESH(%d-%d), screen width: %d\n",
                  backButton->getX(), backButton->getX() + backButton->getWidth(),
                  scanButton->getX(), scanButton->getX() + scanButton->getWidth(),
                  refreshButton->getX(), refreshButton->getX() + refreshButton->getWidth(),
                  lcd->width());
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
        lcd->setTextSize(UIScale::getGeneralTextSize());
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
            lcd->setTextSize(UIScale::getGeneralTextSize());
            lcd->setCursor(indicatorX, infoAreaY + UIScale::scale(5));
            lcd->print("^");
        }

        if (scrollOffset < (int)devices.size() - maxVisibleDevices) {
            lcd->setTextColor(0xFFFF);
            lcd->setTextSize(UIScale::getGeneralTextSize());
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

// Helper function to clip text with ellipsis if it's too long
String DeviceManagerScreen::clipText(const String& text, int maxWidth, int textSize) {
    if (!lcd) return text;

    int textWidth = UIScale::calculateTextWidth(text, textSize);
    if (textWidth <= maxWidth) {
        return text;  // Text fits, no clipping needed
    }

    // Calculate how many characters we can fit with "..." at the end
    int ellipsisWidth = UIScale::calculateTextWidth("...", textSize);
    int availableWidth = maxWidth - ellipsisWidth;

    if (availableWidth <= 0) {
        return "...";  // Not enough space for anything
    }

    // Binary search to find the maximum number of characters that fit
    int left = 0, right = text.length();
    int bestFit = 0;

    while (left <= right) {
        int mid = (left + right) / 2;
        int midWidth = UIScale::calculateTextWidth(text.substring(0, mid), textSize);

        if (midWidth <= availableWidth) {
            bestFit = mid;
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }

    if (bestFit == 0) {
        return "...";
    }

    return text.substring(0, bestFit) + "...";
}

String DeviceManagerScreen::formatDeviceInfo(const DeviceInfo& device) {
    String status = device.connected ? "CONN" : "DISC";
    String signalStr = String(device.rssi) + "dBm";

    // Calculate available width for device name
    // Button width minus space for status and signal strength
    int buttonWidth = lcd->width() - UIScale::scale(20);  // Account for margins
    int statusWidth = UIScale::calculateTextWidth(" [" + status + "] " + signalStr, UIScale::getButtonTextSize());
    int availableForName = buttonWidth - statusWidth - UIScale::scale(16);  // Account for button padding

    // Clip device name if necessary
    String clippedName = clipText(device.name, availableForName, UIScale::getButtonTextSize());

    return clippedName + " [" + status + "] " + signalStr;
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
        bluetoothManager->disconnectDevice(address);
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