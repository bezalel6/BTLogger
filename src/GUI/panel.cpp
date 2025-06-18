#include "panel.hpp"
#include "layout.hpp"
#include <algorithm>
#include "esp_task_wdt.h"

// Initialize static counter
int Panel::s_nextPanelId = 1;

Panel::Panel(Rectangle bounds) : Component(bounds) {
    // Panel is constructed with fixed bounds
    m_id = String(s_nextPanelId++);
}

Panel::Panel() : Component() {
    // Panel will be layout-managed by parent
    m_id = String(s_nextPanelId++);
}

Panel::~Panel() {
    clearComponents();
}

void Panel::addComponent(Component* component) {
    if (component != nullptr) {
        component->parentPanel = this;
        m_components.push_back(component);

        // Trigger layout recalculation
        if (m_layoutType != LayoutType::NONE) {
            calculateLayout();
        }
    }
}

void Panel::removeComponent(Component* component) {
    auto it = std::find(m_components.begin(), m_components.end(), component);
    if (it != m_components.end()) {
        (*it)->parentPanel = nullptr;
        m_components.erase(it);

        // Trigger layout recalculation
        if (m_layoutType != LayoutType::NONE) {
            calculateLayout();
        }
    }
}

void Panel::clearComponents() {
    for (auto* component : m_components) {
        if (component) {
            component->parentPanel = nullptr;
            delete component;
        }
    }
    m_components.clear();
}

void Panel::setLayoutType(LayoutType type) {
    m_layoutType = type;
    // Trigger layout recalculation
    calculateLayout();
}

Rectangle Panel::getContentBounds() const {
    // Return bounds excluding padding
    return Rectangle(
        bounds.origin.x + layoutProps.paddingLeft,
        bounds.origin.y + layoutProps.paddingTop,
        bounds.w - layoutProps.paddingLeft - layoutProps.paddingRight,
        bounds.h - layoutProps.paddingTop - layoutProps.paddingBottom);
}

void Panel::draw(LGFX lcd) {
    if (!needsRedraw) {
        // Still need to draw children even if panel itself doesn't need redraw
        drawComponents(lcd);
        return;
    }

    // Draw background if specified
    drawBackground(lcd);

    // Draw child components
    drawComponents(lcd);

    // Mark as clean
    markClean();
}

void Panel::calculateLayout() {
    if (m_layoutType != LayoutType::NONE) {
        LayoutManager::getInstance().calculateLayout(this, m_layoutType);
    }
}

bool Panel::handleTouch(const TouchCoordinates& touchCoords) {
    if (this->getId().equals("main-panel")) {
        Serial.println("main panel asked to handle touch. returning: true");
        return true;
    }
    // Handle touch for child components (in reverse order for proper layering)
    int i = 0;
    for (auto it = m_components.rbegin(); it != m_components.rend(); ++it, i++) {
        // Feed watchdog to prevent reset during panel touch processing
        esp_task_wdt_reset();
        yield();

        Component* comp = *it;
        if (comp && comp->checkTouching(touchCoords)) {
            comp->markDirty();
            comp->clicked();
            return true;
        }

        // Safety check to prevent infinite loops
        if (i > 20) {
            Serial.println("ERROR: Panel has too many child components, breaking to prevent infinite loop");
            break;
        }
    }

    return false;
}

void Panel::drawBackground(LGFX& lcd) {
    if (m_hasBackground) {
        lcd.fillRect(bounds.origin.x, bounds.origin.y, bounds.w, bounds.h, m_backgroundColor);
    }
}

void Panel::drawComponents(LGFX& lcd) {
    for (auto* component : m_components) {
        if (component != nullptr) {
            component->draw(lcd);
        }
    }
}

void Panel::markAllComponentsDirty() {
    for (auto* component : m_components) {
        if (component != nullptr) {
            component->markDirty();
        }
    }
}