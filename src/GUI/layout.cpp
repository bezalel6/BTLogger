#include "layout.hpp"
#include "panel.hpp"
#include <algorithm>
#include <cmath>

// LayoutManager implementation
LayoutManager& LayoutManager::getInstance() {
    static LayoutManager instance;
    return instance;
}

void LayoutManager::calculateLayout(Panel* panel, LayoutType layoutType) {
    if (!panel) return;

    switch (layoutType) {
        case LayoutType::FLEX_ROW:
        case LayoutType::FLEX_COLUMN:
            m_flexEngine.calculateLayout(panel);
            break;
        case LayoutType::GRID:
            m_gridEngine.calculateLayout(panel);
            break;
        case LayoutType::STACK:
            m_stackEngine.calculateLayout(panel);
            break;
        case LayoutType::NONE:
        default:
            // No automatic layout
            break;
    }
}

// FlexLayoutEngine implementation
void FlexLayoutEngine::calculateLayout(Panel* panel) {
    if (!panel) return;

    if (panel->getLayoutType() == LayoutType::FLEX_ROW) {
        calculateFlexRow(panel);
    } else if (panel->getLayoutType() == LayoutType::FLEX_COLUMN) {
        calculateFlexColumn(panel);
    }
}

void FlexLayoutEngine::calculateFlexRow(Panel* panel) {
    const auto& components = panel->getComponents();
    Rectangle contentBounds = panel->getContentBounds();

    if (components.empty()) return;

    // Calculate total fixed width and flex factors
    int totalFixedWidth = 0;
    int totalFlexGrow = 0;
    int totalGaps = (components.size() - 1) * panel->m_gap;

    for (Component* comp : components) {
        if (!comp) continue;

        const LayoutProps& props = comp->layoutProps;
        totalFixedWidth += props.marginLeft + props.marginRight;

        if (props.widthMode == SizeMode::FIXED) {
            totalFixedWidth += props.width;
        } else if (props.widthMode == SizeMode::PERCENT) {
            totalFixedWidth += static_cast<int>(contentBounds.w * props.widthPercent);
        } else if (props.widthMode == SizeMode::FLEX) {
            totalFlexGrow += props.flexGrow;
        }
    }

    // Calculate remaining space for flex items
    int remainingWidth = std::max(0, contentBounds.w - totalFixedWidth - totalGaps);
    int flexUnit = totalFlexGrow > 0 ? remainingWidth / totalFlexGrow : 0;

    // Position components
    int currentX = contentBounds.origin.x;
    int panelHeight = contentBounds.h;

    for (Component* comp : components) {
        if (!comp) continue;

        const LayoutProps& props = comp->layoutProps;

        // Calculate component width
        int compWidth = 0;
        if (props.widthMode == SizeMode::FIXED) {
            compWidth = props.width;
        } else if (props.widthMode == SizeMode::PERCENT) {
            compWidth = static_cast<int>(contentBounds.w * props.widthPercent);
        } else if (props.widthMode == SizeMode::FLEX) {
            compWidth = flexUnit * props.flexGrow;
        } else if (props.widthMode == SizeMode::AUTO) {
            compWidth = comp->bounds.w;  // Use current width
        }

        // Calculate component height
        int compHeight = 0;
        if (props.heightMode == SizeMode::FIXED) {
            compHeight = props.height;
        } else if (props.heightMode == SizeMode::PERCENT) {
            compHeight = static_cast<int>(panelHeight * props.heightPercent);
        } else if (props.heightMode == SizeMode::FILL || props.heightMode == SizeMode::FLEX) {
            compHeight = panelHeight - props.marginTop - props.marginBottom;
        } else if (props.heightMode == SizeMode::AUTO) {
            compHeight = comp->bounds.h;  // Use current height
        }

        // Apply margins
        currentX += props.marginLeft;

        // Calculate Y position based on alignment
        int compY = contentBounds.origin.y + props.marginTop;
        if (panel->m_alignItems == Alignment::CENTER) {
            compY = contentBounds.origin.y + (panelHeight - compHeight) / 2;
        } else if (panel->m_alignItems == Alignment::END) {
            compY = contentBounds.origin.y + panelHeight - compHeight - props.marginBottom;
        }

        // Set component bounds
        comp->bounds = Rectangle(currentX, compY, compWidth, compHeight);
        comp->markDirty();

        // Move to next position
        currentX += compWidth + props.marginRight + panel->m_gap;
    }
}

void FlexLayoutEngine::calculateFlexColumn(Panel* panel) {
    const auto& components = panel->getComponents();
    Rectangle contentBounds = panel->getContentBounds();

    if (components.empty()) return;

    // Calculate total fixed height and flex factors
    int totalFixedHeight = 0;
    int totalFlexGrow = 0;
    int totalGaps = (components.size() - 1) * panel->m_gap;

    for (Component* comp : components) {
        if (!comp) continue;

        const LayoutProps& props = comp->layoutProps;
        totalFixedHeight += props.marginTop + props.marginBottom;

        if (props.heightMode == SizeMode::FIXED) {
            totalFixedHeight += props.height;
        } else if (props.heightMode == SizeMode::PERCENT) {
            totalFixedHeight += static_cast<int>(contentBounds.h * props.heightPercent);
        } else if (props.heightMode == SizeMode::FLEX) {
            totalFlexGrow += props.flexGrow;
        }
    }

    // Calculate remaining space for flex items
    int remainingHeight = std::max(0, contentBounds.h - totalFixedHeight - totalGaps);
    int flexUnit = totalFlexGrow > 0 ? remainingHeight / totalFlexGrow : 0;

    // Position components
    int currentY = contentBounds.origin.y;
    int panelWidth = contentBounds.w;

    for (Component* comp : components) {
        if (!comp) continue;

        const LayoutProps& props = comp->layoutProps;

        // Calculate component height
        int compHeight = 0;
        if (props.heightMode == SizeMode::FIXED) {
            compHeight = props.height;
        } else if (props.heightMode == SizeMode::PERCENT) {
            compHeight = static_cast<int>(contentBounds.h * props.heightPercent);
        } else if (props.heightMode == SizeMode::FLEX) {
            compHeight = flexUnit * props.flexGrow;
        } else if (props.heightMode == SizeMode::AUTO) {
            compHeight = comp->bounds.h;  // Use current height
        }

        // Calculate component width
        int compWidth = 0;
        if (props.widthMode == SizeMode::FIXED) {
            compWidth = props.width;
        } else if (props.widthMode == SizeMode::PERCENT) {
            compWidth = static_cast<int>(panelWidth * props.widthPercent);
        } else if (props.widthMode == SizeMode::FILL || props.widthMode == SizeMode::FLEX) {
            compWidth = panelWidth - props.marginLeft - props.marginRight;
        } else if (props.widthMode == SizeMode::AUTO) {
            compWidth = comp->bounds.w;  // Use current width
        }

        // Apply margins
        currentY += props.marginTop;

        // Calculate X position based on alignment
        int compX = contentBounds.origin.x + props.marginLeft;
        if (panel->m_alignItems == Alignment::CENTER) {
            compX = contentBounds.origin.x + (panelWidth - compWidth) / 2;
        } else if (panel->m_alignItems == Alignment::END) {
            compX = contentBounds.origin.x + panelWidth - compWidth - props.marginRight;
        }

        // Set component bounds
        comp->bounds = Rectangle(compX, currentY, compWidth, compHeight);
        comp->markDirty();

        // Move to next position
        currentY += compHeight + props.marginBottom + panel->m_gap;
    }
}

// GridLayoutEngine implementation
void GridLayoutEngine::calculateLayout(Panel* panel) {
    if (!panel) return;

    const auto& components = panel->getComponents();
    Rectangle contentBounds = panel->getContentBounds();

    if (components.empty()) return;

    int columns = panel->m_gridColumns;
    int rows = panel->m_gridRows;

    // Calculate cell size
    int cellWidth = contentBounds.w / columns;
    int cellHeight = contentBounds.h / rows;

    for (Component* comp : components) {
        if (!comp) continue;

        const LayoutProps& props = comp->layoutProps;

        // Calculate grid position
        int col = props.gridColumn;
        int row = props.gridRow;
        int colSpan = props.gridColumnSpan;
        int rowSpan = props.gridRowSpan;

        // Ensure valid grid position
        if (col >= columns) col = columns - 1;
        if (row >= rows) row = rows - 1;
        if (col + colSpan > columns) colSpan = columns - col;
        if (row + rowSpan > rows) rowSpan = rows - row;

        // Calculate position and size
        int x = contentBounds.origin.x + col * cellWidth + props.marginLeft;
        int y = contentBounds.origin.y + row * cellHeight + props.marginTop;
        int width = cellWidth * colSpan - props.marginLeft - props.marginRight;
        int height = cellHeight * rowSpan - props.marginTop - props.marginBottom;

        // Set component bounds
        comp->bounds = Rectangle(x, y, width, height);
        comp->markDirty();
    }
}

// StackLayoutEngine implementation
void StackLayoutEngine::calculateLayout(Panel* panel) {
    if (!panel) return;

    const auto& components = panel->getComponents();
    Rectangle contentBounds = panel->getContentBounds();

    if (components.empty()) return;

    // All components get the same bounds (full panel minus padding)
    for (Component* comp : components) {
        if (!comp) continue;

        const LayoutProps& props = comp->layoutProps;

        // Calculate component size and position
        int x = contentBounds.origin.x + props.marginLeft;
        int y = contentBounds.origin.y + props.marginTop;
        int width = contentBounds.w - props.marginLeft - props.marginRight;
        int height = contentBounds.h - props.marginTop - props.marginBottom;

        // Apply size overrides if specified
        if (props.widthMode == SizeMode::FIXED) {
            width = props.width;
        } else if (props.widthMode == SizeMode::PERCENT) {
            width = static_cast<int>(contentBounds.w * props.widthPercent);
        }

        if (props.heightMode == SizeMode::FIXED) {
            height = props.height;
        } else if (props.heightMode == SizeMode::PERCENT) {
            height = static_cast<int>(contentBounds.h * props.heightPercent);
        }

        // Apply alignment
        if (panel->m_alignItems == Alignment::CENTER) {
            x = contentBounds.origin.x + (contentBounds.w - width) / 2;
            y = contentBounds.origin.y + (contentBounds.h - height) / 2;
        } else if (panel->m_alignItems == Alignment::END) {
            x = contentBounds.origin.x + contentBounds.w - width - props.marginRight;
            y = contentBounds.origin.y + contentBounds.h - height - props.marginBottom;
        }

        // Set component bounds
        comp->bounds = Rectangle(x, y, width, height);
        comp->markDirty();
    }
}