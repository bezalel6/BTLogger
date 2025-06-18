#include "GuiManager.hpp"
#include <algorithm>
#include "esp_task_wdt.h"

GuiManager::GuiManager(LGFX& lcd)
    : m_lcd(lcd), m_textSize(DEFAULT_TEXT_SIZE), m_textColor(TFT_WHITE) {
}

// Initialize static member
int GuiManager::s_touchHandlingDepth = 0;

GuiManager::~GuiManager() {
    clearComponents();
}

void GuiManager::init() {
    m_lcd.init();
    m_lcd.setTextSize(m_textSize);
    m_lcd.setRotation(1);

    // Check if we have saved calibration data
    if (hasSavedCalibration()) {
        // Load and apply saved calibration
        std::uint16_t calData[8];  // Standard calibration data array size
        if (loadTouchCalibration(calData)) {
            m_lcd.setTouchCalibrate(calData);
            Serial.println("Touch calibration loaded from storage");
        } else {
            Serial.println("Failed to load touch calibration, performing new calibration");
            performTouchCalibration();
        }
    } else {
        // No saved calibration found, perform calibration
        Serial.println("No saved touch calibration found, performing calibration");
        performTouchCalibration();
    }
}

void GuiManager::update() {
    drawComponents();

    // Rate limit touch handling to prevent overwhelming the system
    static unsigned long lastTouchCheck = 0;
    unsigned long now = millis();
    if (now - lastTouchCheck > 50) {  // Check touch every 50ms
        handleComponentTouch();
        lastTouchCheck = now;
    }
}

void GuiManager::clear() {
    m_lcd.clear();
    markAllComponentsDirty();  // Components need to be redrawn after clearing
}

void GuiManager::addComponent(Component* component) {
    if (component != nullptr) {
        m_components.push_back(component);
    }
}

void GuiManager::removeComponent(Component* component) {
    auto it = std::find(m_components.begin(), m_components.end(), component);
    if (it != m_components.end()) {
        m_components.erase(it);
    }
}

void GuiManager::clearComponents() {
    for (auto* component : m_components) {
        delete component;
    }
    m_components.clear();
}

Button* GuiManager::createButton(int x, int y, int width, int height, const String& text) {
    Rectangle bounds(x, y, width, height);
    return createButton(bounds, text);
}

Button* GuiManager::createButton(Rectangle bounds, const String& text) {
    Button* button = new Button(bounds, text);
    addComponent(button);
    return button;
}

Button* GuiManager::createButton(const String& text) {
    Button* button = new Button(text);
    addComponent(button);
    return button;
}

Label* GuiManager::createLabel(Rectangle bounds, const String& text) {
    Label* label = new Label(bounds, text);
    addComponent(label);
    return label;
}

Label* GuiManager::createLabel(const String& text) {
    Label* label = new Label(text);
    addComponent(label);
    return label;
}

ProgressBar* GuiManager::createProgressBar(Rectangle bounds, float minValue, float maxValue) {
    ProgressBar* progressBar = new ProgressBar(bounds, minValue, maxValue);
    addComponent(progressBar);
    return progressBar;
}

ProgressBar* GuiManager::createProgressBar(float minValue, float maxValue) {
    ProgressBar* progressBar = new ProgressBar(minValue, maxValue);
    addComponent(progressBar);
    return progressBar;
}

Panel* GuiManager::createPanel(Rectangle bounds) {
    Panel* panel = new Panel(bounds);
    addComponent(panel);
    return panel;
}

Panel* GuiManager::createPanel() {
    Panel* panel = new Panel();
    addComponent(panel);
    return panel;
}

Panel* GuiManager::createFlexPanel(LayoutType layoutType) {
    Panel* panel = new Panel();
    panel->setLayoutType(layoutType);
    addComponent(panel);
    return panel;
}

Panel* GuiManager::createGridPanel(int columns, int rows) {
    Panel* panel = new Panel();
    panel->setLayoutType(LayoutType::GRID);
    panel->setGridColumns(columns);
    panel->setGridRows(rows);
    addComponent(panel);
    return panel;
}

void GuiManager::performTouchCalibration() {
    m_lcd.fillScreen(TFT_YELLOW);

    m_lcd.setTextColor(TFT_BLACK);
    m_lcd.setTextSize(4);
    m_lcd.setCursor(70, 110);
    m_lcd.println("SCREEN");
    m_lcd.setCursor(70, 150);
    m_lcd.println("CALIBRATION");

    // Touch calibration for the four corners of the screen
    std::uint16_t fg = TFT_WHITE;
    std::uint16_t bg = TFT_BLACK;
    if (m_lcd.isEPD()) {
        std::swap(fg, bg);
    }

    std::uint16_t calData[8];  // Array to store calibration data
    m_lcd.calibrateTouch(calData, fg, bg, std::max(m_lcd.width(), m_lcd.height()) >> 3);

    // Save the calibration data to preferences
    if (saveTouchCalibration(calData)) {
        Serial.println("Touch calibration saved to storage");
    } else {
        Serial.println("Failed to save touch calibration");
    }

    m_lcd.clear();
    m_lcd.setTextSize(m_textSize);
}

bool GuiManager::saveTouchCalibration(std::uint16_t* calData) {
    if (calData == nullptr) {
        Serial.println("Error: Invalid calibration data");
        return false;
    }

    m_preferences.begin("touch_cal", false);

    // Save calibration data as bytes
    size_t bytesWritten = m_preferences.putBytes("cal_data", calData, 8 * sizeof(std::uint16_t));
    bool success = (bytesWritten == 8 * sizeof(std::uint16_t));

    if (success) {
        // Set a flag to indicate we have valid calibration data
        m_preferences.putBool("cal_valid", true);

        // Log the calibration values for debugging
        Serial.print("Saved calibration data: ");
        for (int i = 0; i < 8; i++) {
            Serial.print(calData[i]);
            if (i < 7) Serial.print(", ");
        }
        Serial.println();
    } else {
        Serial.println("Error: Failed to save calibration data to preferences");
    }

    m_preferences.end();
    return success;
}

bool GuiManager::loadTouchCalibration(std::uint16_t* calData) {
    if (calData == nullptr) {
        Serial.println("Error: Invalid calibration data buffer");
        return false;
    }

    m_preferences.begin("touch_cal", true);  // Read-only mode

    // Check if we have valid calibration data
    bool hasValidData = m_preferences.getBool("cal_valid", false);
    if (!hasValidData) {
        m_preferences.end();
        return false;
    }

    // Load calibration data
    size_t bytesRead = m_preferences.getBytes("cal_data", calData, 8 * sizeof(std::uint16_t));
    bool success = (bytesRead == 8 * sizeof(std::uint16_t));

    if (success) {
        // Log the loaded calibration values for debugging
        Serial.print("Loaded calibration data: ");
        for (int i = 0; i < 8; i++) {
            Serial.print(calData[i]);
            if (i < 7) Serial.print(", ");
        }
        Serial.println();
    } else {
        Serial.println("Error: Failed to load calibration data from preferences");
    }

    m_preferences.end();
    return success;
}

bool GuiManager::hasSavedCalibration() {
    m_preferences.begin("touch_cal", true);  // Read-only mode
    bool hasValid = m_preferences.getBool("cal_valid", false);
    m_preferences.end();
    return hasValid;
}

void GuiManager::clearTouchCalibration() {
    m_preferences.begin("touch_cal", false);
    m_preferences.clear();
    m_preferences.end();
    Serial.println("Touch calibration data cleared");
}

bool GuiManager::processTouchEvents() {
    return handleComponentTouch();
}

void GuiManager::setTextSize(int size) {
    m_textSize = size;
    m_lcd.setTextSize(size);
}

void GuiManager::setTextColor(uint16_t color) {
    m_textColor = color;
    m_lcd.setTextColor(color);
}

void GuiManager::setCursor(int x, int y) {
    m_lcd.setCursor(x, y);
}

void GuiManager::print(const String& text) {
    m_lcd.print(text);
}

void GuiManager::println(const String& text) {
    m_lcd.println(text);
}

void GuiManager::fillScreen(uint16_t color) {
    m_lcd.fillScreen(color);
    markAllComponentsDirty();  // Components need to be redrawn after filling screen
}

int GuiManager::getWidth() const {
    return m_lcd.width();
}

int GuiManager::getHeight() const {
    return m_lcd.height();
}

void GuiManager::drawComponents() {
    for (auto* component : m_components) {
        if (component != nullptr && component->needsRedraw) {
            component->draw(m_lcd);
        }
    }
}

void GuiManager::markAllComponentsDirty() {
    for (auto* component : m_components) {
        if (component != nullptr) {
            component->markDirty();
        }
    }
}

bool GuiManager::handleComponentTouch() {
    // Prevent recursive calls
    if (s_touchHandlingDepth > 0) {
        Serial.println("ERROR: Recursive touch handling detected, aborting");
        return false;
    }

    s_touchHandlingDepth++;

    // Get current touch coordinates from LCD
    m_currentTouch = getTouchCoordinates();

    if (!m_currentTouch.isTouched) {
        s_touchHandlingDepth--;
        // Reset all component debouncing states when no touch is detected
        for (auto* component : m_components) {
            if (component != nullptr) {
                component->resetDebouncing();
            }
        }
        return false;
    }

    // Reduced debug output - only log when touch is first detected
    static bool wasTouching = false;
    if (!wasTouching) {
        Serial.printf("Touch detected at: %d, %d\n", m_currentTouch.x, m_currentTouch.y);
    }
    wasTouching = true;

    bool touchHandled = false;
    // Handle touch for components in reverse order (top to bottom)
    int i = 0;
    for (auto it = m_components.rbegin(); it != m_components.rend(); ++it, i++) {
        // Feed watchdog to prevent reset during touch processing
        esp_task_wdt_reset();
        yield();

        Component* component = *it;
        if (component != nullptr) {
            // Check if it's a Panel and handle its internal touch events first
            if (component->isPanel()) {
                Panel* panel = static_cast<Panel*>(component);
                if (panel->handleTouch(m_currentTouch)) {
                    touchHandled = true;
                    break;
                }
            }
            // If not handled by panel children, check the component itself
            else {
                if (component->checkTouching(m_currentTouch)) {
                    component->markDirty();  // Mark for redraw to show visual feedback
                    component->clicked();
                    touchHandled = true;
                    break;  // Handle only the first touched component
                }
            }
        }

        // Safety check to prevent infinite loops
        if (i > 20) {
            Serial.println("ERROR: Too many components, breaking to prevent infinite loop");
            break;
        }
    }

    // Reset wasTouching flag when touch is no longer detected
    if (!m_currentTouch.isTouched) {
        wasTouching = false;
    }

    s_touchHandlingDepth--;
    return touchHandled;
}

TouchCoordinates GuiManager::getTouchCoordinates() {
    int pos[2] = {0, 0};
    bool isTouched = m_lcd.getTouch(&pos[0], &pos[1]);
    // Serial.printf("Is Touched: %s\n", isTouched ? "TRUE" : "FALSE");
    return TouchCoordinates(pos[0], pos[1], isTouched);
}