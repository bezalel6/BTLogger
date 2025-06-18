#include "ToastManager.hpp"
#include "UIScale.hpp"

namespace BTLogger {
namespace UI {

// Static member definitions
bool ToastManager::initialized = false;
lgfx::LGFX_Device* ToastManager::lcd = nullptr;
String ToastManager::currentMessage = "";
ToastManager::ToastType ToastManager::currentType = INFO;
unsigned long ToastManager::showTime = 0;
bool ToastManager::visible = false;

void ToastManager::initialize(lgfx::LGFX_Device& display) {
    if (initialized) {
        return;
    }

    lcd = &display;
    initialized = true;
    Serial.println("ToastManager initialized");
}

void ToastManager::update() {
    if (!initialized || !visible) {
        return;
    }

    // Check if toast should be hidden
    if (millis() - showTime > TOAST_DURATION) {
        visible = false;
        // Could add fade-out animation here
    } else {
        drawToast();
    }
}

void ToastManager::showInfo(const String& message) {
    showToast(message, INFO);
}

void ToastManager::showSuccess(const String& message) {
    showToast(message, SUCCESS);
}

void ToastManager::showWarning(const String& message) {
    showToast(message, WARNING);
}

void ToastManager::showError(const String& message) {
    showToast(message, ERROR);
}

void ToastManager::showToast(const String& message, ToastType type) {
    if (!initialized) return;

    currentMessage = message;
    currentType = type;
    showTime = millis();
    visible = true;

    Serial.printf("Toast: %s\n", message.c_str());
}

void ToastManager::drawToast() {
    if (!lcd || !visible) return;

    int width = UIScale::scale(200);
    int height = UIScale::scale(40);
    int x = (240 - width) / 2;
    int y = UIScale::scale(20);

    // Background
    lcd->fillRoundRect(x, y, width, height, UIScale::scale(5), getToastColor(currentType));

    // Border
    lcd->drawRoundRect(x, y, width, height, UIScale::scale(5), 0xFFFF);

    // Text
    lcd->setTextColor(0xFFFF);
    lcd->setTextSize(1);
    lcd->setCursor(x + UIScale::scale(10), y + UIScale::scale(15));
    lcd->print(currentMessage.substring(0, 25));  // Limit length
}

uint16_t ToastManager::getToastColor(ToastType type) {
    switch (type) {
        case SUCCESS:
            return 0x07E0;  // Green
        case WARNING:
            return 0xFFE0;  // Yellow
        case ERROR:
            return 0xF800;  // Red
        default:
            return 0x001F;  // Blue
    }
}

}  // namespace UI
}  // namespace BTLogger