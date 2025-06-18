#pragma once

#include "Arduino.h"
#include "component.hpp"
#include "layout.hpp"
#include <vector>
#include <memory>

class Panel : public Component {
   public:
    // Constructors
    Panel(Rectangle bounds);
    Panel();  // For layout-managed panels

    ~Panel();

    // Component management
    void addComponent(Component* component);
    void removeComponent(Component* component);
    void clearComponents();

    // Layout management
    void setLayoutType(LayoutType type);
    LayoutType getLayoutType() const { return m_layoutType; }

    // Layout properties for child components
    void setJustifyContent(Alignment alignment) { m_justifyContent = alignment; }
    void setAlignItems(Alignment alignment) { m_alignItems = alignment; }
    void setGap(int gap) { m_gap = gap; }

    // Grid-specific properties
    void setGridColumns(int columns) { m_gridColumns = columns; }
    void setGridRows(int rows) { m_gridRows = rows; }

    // ID management
    void setId(const String& id) { m_id = id; }
    String getId() const { return m_id; }

    // Drawing
    void draw(LGFX lcd) override;

    // Type identification
    bool isPanel() const override { return true; }

    // Layout calculation
    void calculateLayout();

    // Touch handling
    bool handleTouch(const TouchCoordinates& touchCoords);

    // Getters
    const std::vector<Component*>& getComponents() const { return m_components; }
    Rectangle getContentBounds() const;

    // Background color
    void setBackgroundColor(uint16_t color) {
        m_backgroundColor = color;
        m_hasBackground = true;
        markDirty();
    }
    void clearBackgroundColor() {
        m_hasBackground = false;
        markDirty();
    }

    // Convenience methods for layout properties
    void setPadding(int padding) { layoutProps.setPadding(padding); }
    void setMargin(int margin) { layoutProps.setMargin(margin); }

   private:
    static int s_nextPanelId;  // Static counter for automatic ID generation

    std::vector<Component*> m_components;
    LayoutType m_layoutType = LayoutType::NONE;
    String m_id;  // Panel ID string

    // Layout alignment properties
    Alignment m_justifyContent = Alignment::START;  // Main axis alignment
    Alignment m_alignItems = Alignment::START;      // Cross axis alignment
    int m_gap = 0;                                  // Gap between components

    // Grid properties
    int m_gridColumns = 1;
    int m_gridRows = 1;

    // Background
    bool m_hasBackground = false;
    uint16_t m_backgroundColor = 0x0000;

    // Helper methods
    void drawBackground(LGFX& lcd);
    void drawComponents(LGFX& lcd);
    void markAllComponentsDirty();

    friend class LayoutEngine;
    friend class FlexLayoutEngine;
    friend class GridLayoutEngine;
    friend class StackLayoutEngine;
};