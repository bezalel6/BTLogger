#pragma once

#include <Arduino.h>
#include <Preferences.h>

namespace BTLogger {
namespace UI {

/**
 * UIScale manages UI element scaling for responsive design
 * Provides adaptive scaling based on screen size and user preferences
 */
class UIScale {
   public:
    // Scale levels
    static const float SCALE_TINY;    // 0.8x - Compact display
    static const float SCALE_SMALL;   // 0.9x - Small elements
    static const float SCALE_NORMAL;  // 1.0x - Default size
    static const float SCALE_LARGE;   // 1.2x - Larger elements
    static const float SCALE_HUGE;    // 1.5x - Maximum size

    // Initialization
    static void initialize();
    static bool isInitialized() { return initialized; }

    // Scale management
    static float getScale() { return currentScale; }
    static void setScale(float scale);
    static void incrementScale();
    static void decrementScale();
    static void resetScale();

    // Convenience methods
    static int scale(int value) { return (int)(value * currentScale); }
    static float scale(float value) { return value * currentScale; }
    static int scaleX(int x) { return scale(x); }
    static int scaleY(int y) { return scale(y); }
    static int scaleWidth(int w) { return scale(w); }
    static int scaleHeight(int h) { return scale(h); }

    // Font scaling
    static int scaleFontSize(int baseFontSize) { return scale(baseFontSize); }

    // Common UI element sizes (pre-scaled)
    static int getButtonHeight() { return scale(40); }
    static int getButtonWidth() { return scale(100); }
    static int getMenuButtonHeight() { return scale(50); }
    static int getMenuButtonWidth() { return scale(180); }
    static int getMargin() { return scale(10); }
    static int getPadding() { return scale(8); }
    static int getIconSize() { return scale(24); }

    // Scale information
    static String getScaleDescription();
    static float getMinScale() { return SCALE_TINY; }
    static float getMaxScale() { return SCALE_HUGE; }

    // Text sizing (independent of layout scale)
    static void setLabelTextSize(int size);
    static void setButtonTextSize(int size);
    static void setGeneralTextSize(int size);

    static int getLabelTextSize();
    static int getButtonTextSize();
    static int getGeneralTextSize();

    // Text width calculation helpers
    static int calculateTextWidth(const String& text, int textSize);
    static int calculateTextHeight(int textSize);

    // Save/load settings
    static void saveSettings();
    static void loadSettings();

   private:
    static bool initialized;
    static float currentScale;
    static Preferences preferences;
    static int labelTextSize;
    static int buttonTextSize;
    static int generalTextSize;

    // Default values
    static const float DEFAULT_SCALE;
    static const int DEFAULT_LABEL_TEXT_SIZE;
    static const int DEFAULT_BUTTON_TEXT_SIZE;
    static const int DEFAULT_GENERAL_TEXT_SIZE;

    // Font width estimation (characters per pixel at size 1)
    static const int CHAR_WIDTH_SIZE_1;
    static const int CHAR_HEIGHT_SIZE_1;

    static void saveScale();
    static void loadScale();
    static float clampScale(float scale);
};

}  // namespace UI
}  // namespace BTLogger