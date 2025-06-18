// Example integration of Input System with existing UniMixTouch project
#include <SPI.h>
#include <vector>

#include "ESP32_SPI_9341.h"
#include "GUI/GuiManager.hpp"
#include "InputSystem.hpp"    // Our new Input System
#include "InputExamples.hpp"  // Example configurations

using namespace std;

#define SD_SCK 18
#define SD_MISO 19
#define SD_MOSI 23
#define SD_CS 5
#define DEFAULT_TEXT_SIZE 3
#define LIGHT_ADC 34

// Hardware Input Pins (adjust these to your actual wiring)
#define ENCODER_PIN_A 27
#define ENCODER_PIN_B 22
#define ENCODER_BUTTON_PIN 5

int led_pin[3] = {17, 4, 16};

LGFX lcd;
GuiManager guiManager(lcd);

// Global UI components for the app
ProgressBar* progressBar = nullptr;
Label* valueLabel = nullptr;
Button* plusButton = nullptr;
Button* minusButton = nullptr;

// App state
float currentProgress = 50.0f;  // Start at 50%

// Input device IDs (set during setup)
uint8_t volumeEncoderId = 0;

// Function declarations
void onPlusButtonClick();
void onMinusButtonClick();
void updateProgressDisplay();
void led_set(int i);
void setupInputDevices();
void handleInputEvents(const InputEvent& event);

void setup(void) {
    pinMode(led_pin[0], OUTPUT);
    pinMode(led_pin[1], OUTPUT);
    pinMode(led_pin[2], OUTPUT);

    Serial.begin(115200);
    Serial.println("UniMixTouch starting...");

    // Initialize GUI Manager (existing code)
    guiManager.init();
    guiManager.fillScreen(TFT_BLACK);

    // === SETUP GUI (existing code) ===
    Panel* mainPanel = guiManager.createFlexPanel(LayoutType::FLEX_COLUMN);
    mainPanel->setId("main-panel");
    mainPanel->bounds = Rectangle(0, 0, guiManager.getWidth(), guiManager.getHeight());
    mainPanel->setBackgroundColor(TFT_BLACK);
    mainPanel->setPadding(20);
    mainPanel->setGap(10);
    mainPanel->setAlignItems(Alignment::CENTER);

    // App title
    Label* titleLabel = guiManager.createLabel("UniMixTouch - Input System Demo");
    titleLabel->layoutProps.heightMode = SizeMode::FIXED;
    titleLabel->layoutProps.height = 35;
    titleLabel->layoutProps.widthMode = SizeMode::FILL;
    titleLabel->setTextColor(TFT_CYAN);
    titleLabel->setTextSize(3);
    titleLabel->setAlignment(Alignment::CENTER);
    mainPanel->addComponent(titleLabel);

    // Current value display
    valueLabel = guiManager.createLabel("Value: 50.0");
    valueLabel->layoutProps.heightMode = SizeMode::FIXED;
    valueLabel->layoutProps.height = 25;
    valueLabel->layoutProps.widthMode = SizeMode::FILL;
    valueLabel->setTextColor(TFT_WHITE);
    valueLabel->setBackgroundColor(TFT_BLACK);
    valueLabel->setTextSize(2);
    valueLabel->setAlignment(Alignment::CENTER);
    mainPanel->addComponent(valueLabel);

    // Progress bar
    progressBar = guiManager.createProgressBar(0.0f, 100.0f);
    progressBar->layoutProps.heightMode = SizeMode::FIXED;
    progressBar->layoutProps.height = 40;
    progressBar->layoutProps.widthMode = SizeMode::FILL;
    progressBar->setColors(TFT_DARKGRAY, TFT_GREEN, TFT_WHITE);
    progressBar->setBorderWidth(2);
    progressBar->setTextColor(TFT_BLACK);
    progressBar->setValue(currentProgress);
    mainPanel->addComponent(progressBar);

    // Control buttons panel (keep existing touch buttons)
    Panel* buttonPanel = guiManager.createFlexPanel(LayoutType::FLEX_ROW);
    buttonPanel->layoutProps.heightMode = SizeMode::FIXED;
    buttonPanel->layoutProps.height = 60;
    buttonPanel->layoutProps.widthMode = SizeMode::FILL;
    buttonPanel->setGap(20);
    buttonPanel->setAlignItems(Alignment::CENTER);
    buttonPanel->setJustifyContent(Alignment::CENTER);
    mainPanel->addComponent(buttonPanel);

    // Minus button
    minusButton = guiManager.createButton("-");
    minusButton->layoutProps.widthMode = SizeMode::FIXED;
    minusButton->layoutProps.width = 80;
    minusButton->layoutProps.heightMode = SizeMode::FILL;
    minusButton->setColors(TFT_RED, TFT_WHITE);
    buttonPanel->addComponent(minusButton);

    // Plus button
    plusButton = guiManager.createButton("+");
    plusButton->layoutProps.widthMode = SizeMode::FIXED;
    plusButton->layoutProps.width = 80;
    plusButton->layoutProps.heightMode = SizeMode::FILL;
    plusButton->setColors(TFT_GREEN, TFT_WHITE);
    buttonPanel->addComponent(plusButton);

    // Instructions
    Label* instructionLabel = guiManager.createLabel("Touch +/- buttons OR use hardware encoder/buttons");
    instructionLabel->layoutProps.heightMode = SizeMode::FIXED;
    instructionLabel->layoutProps.height = 20;
    instructionLabel->layoutProps.widthMode = SizeMode::FILL;
    instructionLabel->setTextColor(TFT_DARKGRAY);
    instructionLabel->setTextSize(1);
    instructionLabel->setAlignment(Alignment::CENTER);
    mainPanel->addComponent(instructionLabel);

    // Set up GUI button callbacks
    plusButton->onClick = onPlusButtonClick;
    minusButton->onClick = onMinusButtonClick;

    // Force layout calculation
    mainPanel->calculateLayout();

    // === SETUP INPUT SYSTEM (NEW) ===
    setupInputDevices();

    // Initialize both systems
    INPUT_SETUP();  // Initialize Input System

    // Initialize the display
    updateProgressDisplay();

    Serial.println("UniMixTouch with Input System initialized!");
}

void setupInputDevices() {
    Serial.println("Setting up hardware input devices...");
    // Setup rotary encoder with button for volume control
    EncoderConfig volumeEncoder(ENCODER_PIN_A, ENCODER_PIN_B, ENCODER_BUTTON_PIN, true, 0, 100, 50);
    volumeEncoderId = INPUT_SYSTEM.addRotaryEncoder(volumeEncoder);

    // Set up global input event handler
    INPUT_SYSTEM.setGlobalCallback(handleInputEvents);

    Serial.println("Hardware input devices configured!");
}

void handleInputEvents(const InputEvent& event) {
    Serial.printf("Input Event: Device %d, Type %d, Value %ld\n",
                  event.deviceId, (int)event.type, event.value);

    // Handle encoder events
    if (event.deviceId == volumeEncoderId) {
        switch (event.type) {
            case InputEventType::ENCODER_CLOCKWISE:
                currentProgress += 2.0f;  // Smaller increments for encoder
                if (currentProgress > 100.0f) currentProgress = 100.0f;
                updateProgressDisplay();
                Serial.printf("Encoder CW - Progress: %.1f\n", currentProgress);
                break;

            case InputEventType::ENCODER_COUNTERCLOCKWISE:
                currentProgress -= 2.0f;
                if (currentProgress < 0.0f) currentProgress = 0.0f;
                updateProgressDisplay();
                Serial.printf("Encoder CCW - Progress: %.1f\n", currentProgress);
                break;

            case InputEventType::BUTTON_CLICK:
                // Reset to middle
                currentProgress = 50.0f;
                updateProgressDisplay();
                Serial.println("Encoder button clicked - Reset to 50%");
                break;

            case InputEventType::BUTTON_LONG_PRESS:
                // Reset to zero
                currentProgress = 0.0f;
                updateProgressDisplay();
                Serial.println("Encoder button long press - Reset to 0%");
                break;
        }
    }
}

void loop(void) {
    // Update GUI (existing code)
    guiManager.update();

    // Update Input System (NEW - this is all you need!)
    INPUT_LOOP();

    delay(10);  // Reduced delay for more responsive input
}

// Existing button callback functions
void onPlusButtonClick() {
    currentProgress += 5.0f;
    if (currentProgress > 100.0f) {
        currentProgress = 100.0f;
    }
    updateProgressDisplay();
    Serial.println("GUI Plus button clicked - Progress: " + String(currentProgress));
}

void onMinusButtonClick() {
    currentProgress -= 5.0f;
    if (currentProgress < 0.0f) {
        currentProgress = 0.0f;
    }
    updateProgressDisplay();
    Serial.println("GUI Minus button clicked - Progress: " + String(currentProgress));
}

void updateProgressDisplay() {
    if (progressBar) {
        progressBar->setValue(currentProgress);
    }
    if (valueLabel) {
        valueLabel->setText("Value: " + String(currentProgress, 1) + "%");
        valueLabel->markDirty();
    }

    // Control LEDs based on progress level
    if (currentProgress <= 33.0f) {
        led_set(0);  // Red LED
    } else if (currentProgress <= 66.0f) {
        led_set(1);  // Green LED
    } else {
        led_set(2);  // Blue LED
    }
}

void led_set(int i) {
    if (i == 0) {
        digitalWrite(led_pin[0], LOW);
        digitalWrite(led_pin[1], HIGH);
        digitalWrite(led_pin[2], HIGH);
    }
    if (i == 1) {
        digitalWrite(led_pin[0], HIGH);
        digitalWrite(led_pin[1], LOW);
        digitalWrite(led_pin[2], HIGH);
    }
    if (i == 2) {
        digitalWrite(led_pin[0], HIGH);
        digitalWrite(led_pin[1], HIGH);
        digitalWrite(led_pin[2], LOW);
    }
}