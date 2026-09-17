#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <string.h>

#include "app_ids.h"
#include "ui_layout.h"

static int scaled(const UiLayout *layout, int value) {
    return MulDiv(value, layout->dpi ? layout->dpi : 96, 96);
}

void ui_layout_init(UiLayout *layout, int dpi) {
    memset(layout, 0, sizeof(*layout));
    layout->dpi = dpi > 0 ? dpi : 96;
}

void ui_layout_add(UiLayout *layout, HWND window, int id, int x, int y, int width, int height, int advanced) {
    UiLayoutItem *item;
    if (!layout || !window || layout->count >= UI_LAYOUT_MAX_ITEMS) return;
    item = &layout->items[layout->count++];
    item->window = window;
    item->id = id;
    item->x = x;
    item->y = y;
    item->width = width;
    item->height = height;
    item->advanced = advanced;
}

void ui_layout_set_base(UiLayout *layout, HWND parent) {
    RECT client;
    GetClientRect(parent, &client);
    layout->base_width = client.right;
    layout->base_height = client.bottom;
}

static void move_item(const UiLayoutItem *item, int x, int y, int width, int height) {
    MoveWindow(item->window, x, y, width > 1 ? width : 1, height > 1 ? height : 1, TRUE);
}

void ui_layout_resize(UiLayout *layout, int client_width, int client_height, int advanced_threshold) {
    int i;
    int dx;
    int half;
    int right_growth;
    int show_advanced;
    int log_y;
    int log_height;
    if (!layout || layout->base_width <= 0 || layout->base_height <= 0) return;
    dx = client_width - layout->base_width;
    half = dx / 2;
    right_growth = dx - half;
    show_advanced = client_height >= advanced_threshold;
    layout->advanced_visible = show_advanced;
    log_y = show_advanced ? scaled(layout, 634) : scaled(layout, 534);
    log_height = client_height - log_y - scaled(layout, 36);
    if (log_height < scaled(layout, 70)) log_height = scaled(layout, 70);

    for (i = 0; i < layout->count; ++i) {
        const UiLayoutItem *item = &layout->items[i];
        int x = item->x;
        int y = item->y;
        int width = item->width;
        int height = item->height;

        if (item->advanced) {
            ShowWindow(item->window, show_advanced ? SW_SHOW : SW_HIDE);
            if (!show_advanced) continue;
            if (item->x == scaled(layout, 10) && item->width > scaled(layout, 1000)) width += dx;
            else if (item->id == ID_TOOLS_POWERSHELL || item->id == ID_TOOLS_CMD || item->id == ID_TOOLS_OPEN_ADVANCED) x += dx;
            move_item(item, x, y, width, height);
            continue;
        }
        if (item->id == ID_STATUS_LINE) {
            move_item(item, x, client_height - scaled(layout, 31), width + dx, height);
            continue;
        }
        if (item->id == ID_GROUP_HEADER_LOG) {
            move_item(item, x, log_y - scaled(layout, 2), width, height);
            continue;
        }
        if (item->id == ID_GROUP_HEADER_DEVICE) {
            move_item(item, x + half, y, width, height);
            continue;
        }
        if (item->id == ID_LOG_LIST) {
            move_item(item, x, log_y + scaled(layout, 28), width + dx, log_height - scaled(layout, 49));
            continue;
        }
        if (item->y == scaled(layout, 534) && item->x == scaled(layout, 10) &&
            item->width > scaled(layout, 1000)) {
            move_item(item, x, log_y, width + dx, log_height);
            continue;
        }
        if (item->y < scaled(layout, 100)) {
            if (item->x == scaled(layout, 10) && item->width > scaled(layout, 1000)) width += dx;
            else if (item->id == ID_ADAPTER_COMBO) width += half;
            else if (item->id == ID_REFRESH || item->id == ID_ADAPTER_STATUS || item->id == ID_ADAPTER_LINK) x += half;
            else if (item->id == ID_ADAPTER_DESC) width += half;
            else if (item->id == ID_ADAPTER_ADDRS) {
                x += half;
                width += right_growth;
            } else if (item->x >= scaled(layout, 1015)) x += dx;
            move_item(item, x, y, width, height);
            continue;
        }
        if (item->y >= scaled(layout, 102) && item->y < scaled(layout, 530)) {
            if (item->x < scaled(layout, 580)) {
                if (item->x == scaled(layout, 10) && item->width > scaled(layout, 500)) width += half;
                else if (item->id == ID_PROFILE_COMBO || item->id == ID_PROFILE_NAME ||
                         item->id == ID_SECONDARY_LIST) width += half;
                else if (item->x >= scaled(layout, 275)) x += half;
            } else {
                x += half;
                if (item->x == scaled(layout, 585) && item->width > scaled(layout, 500)) width += right_growth;
                else if (item->id == ID_DEVICE_LIST || item->id == ID_DEVICE_RESULT ||
                         item->width > scaled(layout, 500)) {
                    width += right_growth;
                } else if (item->y == scaled(layout, 327) && item->id == ID_DEVICE_NAME) {
                    width += right_growth / 2;
                } else if (item->y == scaled(layout, 327) && item->x >= scaled(layout, 855)) {
                    x += right_growth / 2;
                    if (item->id == ID_DEVICE_IP) width += right_growth - right_growth / 2;
                }
            }
            move_item(item, x, y, width, height);
        }
    }
}
