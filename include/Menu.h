#ifndef MENU_H
#define MENU_H

class Menu {
public:
    int getSelectedLine() const;
    int getSelectedEntryIndex() const;
    int getTopEntryIndex() const;
    int getMenuLines() const;
    int getMenuObjects() const;

    void setMenuExtent(int nObjects, int maxlines);

    void setSelectedLine(int pos);
    void moveDown();
    void moveUp();
    void pageDown();
    void pageUp();
    void goTop();
    void goBottom();

private:
    int selected_pos = 0;
    int menu_scroll_pos = 0;
    int m_nObjects = -1;
    int m_nLines = -1;
};

#endif // MENU_H
