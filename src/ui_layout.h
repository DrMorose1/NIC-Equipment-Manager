#ifndef NIC_EQUIPMENT_MANAGER_UI_LAYOUT_H
#define NIC_EQUIPMENT_MANAGER_UI_LAYOUT_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define UI_LAYOUT_MAX_ITEMS 128

typedef struct {
    HWND window;
    int id;
    int x;
    int y;
    int width;
    int height;
    int advanced;
} UiLayoutItem;

typedef struct {
    UiLayoutItem items[UI_LAYOUT_MAX_ITEMS];
    int count;
    int base_width;
    int base_height;
    int dpi;
    int advanced_visible;
} UiLayout;

void ui_layout_init(UiLayout *layout, int dpi);
void ui_layout_add(UiLayout *layout, HWND window, int id, int x, int y, int width, int height, int advanced);
void ui_layout_set_base(UiLayout *layout, HWND parent);
void ui_layout_resize(UiLayout *layout, int client_width, int client_height, int advanced_threshold);

#endif
