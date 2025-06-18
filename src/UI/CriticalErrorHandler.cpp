#include "CriticalErrorHandler.hpp"
#include "UIScale.hpp"

namespace BTLogger {
namespace UI {

// Static member definitions
bool CriticalErrorHandler::initialized = false;
lgfx::LGFX_Device* CriticalErrorHandler::lcd = nullptr;

void CriticalErrorHandler::initialize(lgfx::LGFX_Device& display) {
    if (initialized) {
        return;
    }

    lcd = &display;
    initialized = true;
    Serial.println("CriticalErrorHandler initialized");
}

void CriticalErrorHandler::showCriticalError(const String& error, const String& details) {
    if (!lcd) return;

    // Blue screen of death style
    lcd->fillScreen(0x001F);  // Blue background

    lcd->setTextColor(0xFFFF);  // White text
    lcd->setTextSize(2);
    lcd->setCursor(UIScale::scale(20), UIScale::scale(50));
    lcd->print("CRITICAL ERROR");

    lcd->setTextSize(1);
    lcd->setCursor(UIScale::scale(20), UIScale::scale(80));
    lcd->print(error.substring(0, 30));

    if (details.length() > 0) {
        lcd->setCursor(UIScale::scale(20), UIScale::scale(100));
        lcd->print(details.substring(0, 30));
    }

    lcd->setCursor(UIScale::scale(20), UIScale::scale(140));
    lcd->print("Touch to restart");

    Serial.printf("CRITICAL ERROR: %s - %s\n", error.c_str(), details.c_str());
}

void CriticalErrorHandler::handleFatalError(const String& error) {
    showCriticalError("FATAL ERROR", error);

    // In a real implementation, this would wait for touch and restart
    delay(5000);
    ESP.restart();
}

}  // namespace UI
}  // namespace BTLogger