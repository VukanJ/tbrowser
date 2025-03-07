#include "Menu.h"
#include <algorithm>
#include <ncurses.h>


int Menu::getSelectedLine() const {
    return selected_pos;
}

int Menu::getSelectedEntryIndex() const {
    return selected_pos + menu_scroll_pos;
}

int Menu::getTopEntryIndex() const {
    return menu_scroll_pos;
}

int Menu::getMenuLines() const {
    return m_nLines;
}

int Menu::getMenuObjects() const {
    return m_nObjects;
}

void Menu::setMenuExtent(int nObjects, int maxLines) {
    m_nObjects = nObjects;
    m_nLines = maxLines;
    if (selected_pos > maxLines) {
        selected_pos = maxLines - 1;
    }
    if (getSelectedEntryIndex() >= m_nObjects) {
        // Can't select nothing
        selected_pos = m_nObjects - 1;
    }
}

void Menu::setSelectedLine(int pos) {
    selected_pos = pos;
}

void Menu::moveDown() { 
    // Can we move down?
    if (getSelectedEntryIndex() >= m_nObjects - 1) {
        // Reached end of root file
        return;
    }

    if (selected_pos < m_nLines - 1) {
        selected_pos++;
    }
    else {
        menu_scroll_pos++;
    }
}

void Menu::moveUp() {
    if (selected_pos == 0) {
        if (menu_scroll_pos > 0) {
            menu_scroll_pos--;
        }
    }
    else {
        selected_pos = std::max<int>(0, selected_pos - 1);
    }
}

void Menu::goTop() {
    selected_pos = 0;
    menu_scroll_pos = 0;
}

void Menu::goBottom() { 
    if (m_nObjects < m_nLines) {
        menu_scroll_pos = 0;
        selected_pos = m_nObjects - 1;
    }
    else {
        selected_pos = m_nLines - 1;
        menu_scroll_pos = m_nObjects - m_nLines;
    }
}
