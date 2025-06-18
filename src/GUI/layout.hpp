#pragma once

#include "../utils.hpp"
#include "Arduino.h"
#include <vector>

// Forward declarations
class Component;
class Panel;

// Layout types
enum class LayoutType {
    NONE,         // No automatic layout (absolute positioning)
    FLEX_ROW,     // Horizontal flex layout
    FLEX_COLUMN,  // Vertical flex layout
    GRID,         // Grid layout
    STACK         // Stack layout (components on top of each other)
};

// Sizing modes for dynamic sizing
enum class SizeMode {
    FIXED,    // Fixed pixel size
    PERCENT,  // Percentage of parent container
    FLEX,     // Flexible size (grows/shrinks)
    AUTO,     // Size based on content
    FILL      // Fill remaining space
};

// Alignment options
enum class Alignment {
    START,   // Left/Top alignment
    CENTER,  // Center alignment
    END,     // Right/Bottom alignment
    STRETCH  // Stretch to fill
};

// Layout properties for spacing and positioning
struct LayoutProps {
    // Margins (space outside the component)
    int marginTop = 0;
    int marginRight = 0;
    int marginBottom = 0;
    int marginLeft = 0;

    // Padding (space inside the component)
    int paddingTop = 0;
    int paddingRight = 0;
    int paddingBottom = 0;
    int paddingLeft = 0;

    // Size properties
    SizeMode widthMode = SizeMode::FIXED;
    SizeMode heightMode = SizeMode::FIXED;
    int width = 0;               // Used when widthMode is FIXED
    int height = 0;              // Used when heightMode is FIXED
    float widthPercent = 1.0f;   // Used when widthMode is PERCENT (0.0-1.0)
    float heightPercent = 1.0f;  // Used when heightMode is PERCENT (0.0-1.0)
    int flexGrow = 0;            // Flex grow factor
    int flexShrink = 1;          // Flex shrink factor

    // Alignment
    Alignment alignSelf = Alignment::STRETCH;

    // Grid properties (for grid layout)
    int gridColumn = 0;      // Grid column position
    int gridRow = 0;         // Grid row position
    int gridColumnSpan = 1;  // Number of columns to span
    int gridRowSpan = 1;     // Number of rows to span

    // Helper functions
    void setMargin(int all) {
        marginTop = marginRight = marginBottom = marginLeft = all;
    }

    void setMargin(int vertical, int horizontal) {
        marginTop = marginBottom = vertical;
        marginLeft = marginRight = horizontal;
    }

    void setPadding(int all) {
        paddingTop = paddingRight = paddingBottom = paddingLeft = all;
    }

    void setPadding(int vertical, int horizontal) {
        paddingTop = paddingBottom = vertical;
        paddingLeft = paddingRight = horizontal;
    }

    void setSize(int w, int h) {
        widthMode = SizeMode::FIXED;
        heightMode = SizeMode::FIXED;
        width = w;
        height = h;
    }

    void setSizePercent(float w, float h) {
        widthMode = SizeMode::PERCENT;
        heightMode = SizeMode::PERCENT;
        widthPercent = w;
        heightPercent = h;
    }

    void setFlex(int grow = 1, int shrink = 1) {
        widthMode = SizeMode::FLEX;
        heightMode = SizeMode::FLEX;
        flexGrow = grow;
        flexShrink = shrink;
    }
};

// Layout engine interface
class LayoutEngine {
   public:
    virtual ~LayoutEngine() = default;
    virtual void calculateLayout(Panel* panel) = 0;
};

// Flex layout engine
class FlexLayoutEngine : public LayoutEngine {
   public:
    void calculateLayout(Panel* panel) override;

   private:
    void calculateFlexRow(Panel* panel);
    void calculateFlexColumn(Panel* panel);
};

// Grid layout engine
class GridLayoutEngine : public LayoutEngine {
   public:
    void calculateLayout(Panel* panel) override;
};

// Stack layout engine
class StackLayoutEngine : public LayoutEngine {
   public:
    void calculateLayout(Panel* panel) override;
};

// Layout manager - manages different layout engines
class LayoutManager {
   public:
    static LayoutManager& getInstance();

    void calculateLayout(Panel* panel, LayoutType layoutType);

   private:
    FlexLayoutEngine m_flexEngine;
    GridLayoutEngine m_gridEngine;
    StackLayoutEngine m_stackEngine;
};