#define _WIN32_WINNT 0x0601
#define WIN32_LEAN_AND_MEAN
#define MAIN_MIN_CLIENT_WIDTH 1180
#define MAIN_MIN_CLIENT_HEIGHT 745

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <commctrl.h>
#include <ipexport.h>
#include <icmpapi.h>
#include <iphlpapi.h>
#include <shellapi.h>
#include <shlobj.h>
#include <uxtheme.h>
#include <dwmapi.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "config.h"
#include "app_ids.h"
#include "../resource.h"
#include "shell_tools.h"
#include "ui_layout.h"
#include "validation.h"
#include "diagnostics.h"
#include "network_commands.h"

#define APP_TITLE "NIC Equipment Manager"
#define APP_VERSION "1.3.3"
#define ADVANCED_PANEL_MIN_CLIENT_HEIGHT 730
#define MAX_ADAPTERS 32
#define MAX_ADAPTER_IPS 16

typedef struct {
    char name[256];
    char description[256];
    char adapter_id[128];
    ULONG if_index;
    IF_OPER_STATUS operational_status;
    ULONG64 link_speed;
    int dhcp_enabled;
    char ip[MAX_ADAPTER_IPS][32];
    int prefix[MAX_ADAPTER_IPS];
    int ip_count;
    char gateway[32];
    char dns1[32];
    char dns2[32];
} AdapterInfo;

static HINSTANCE g_instance;
static HWND g_main;
static HFONT g_font;
static HBRUSH g_background_brush;
static HBRUSH g_edit_brush;
static HBRUSH g_button_brush;
static HBRUSH g_button_pressed_brush;
static COLORREF g_background_color;
static COLORREF g_edit_color;
static COLORREF g_button_color;
static COLORREF g_button_pressed_color;
static COLORREF g_text_color;
static COLORREF g_muted_text_color;
static COLORREF g_border_color;
static int g_dark_theme;
static int g_dpi = 96;
static int g_register_advanced;
static int g_config_load_ok = 1;
static UiLayout g_layout;
static AppConfig g_config;
static AdapterInfo g_adapters[MAX_ADAPTERS];
static int g_adapter_count;
static char g_config_path[MAX_PATH];
static char g_log_path[MAX_PATH];
static char g_backup_path[MAX_PATH];

static HWND h_adapter_combo;
static HWND h_adapter_status;
static HWND h_adapter_link;
static HWND h_adapter_desc;
static HWND h_adapter_addrs;
static HWND h_theme_combo;

static HWND h_profile_combo;
static HWND h_profile_name;
static HWND h_dhcp;
static HWND h_primary_ip;
static HWND h_prefix;
static HWND h_gateway;
static HWND h_dns1;
static HWND h_dns2;
static HWND h_secondary_list;
static HWND h_secondary_ip;
static HWND h_secondary_prefix;

static HWND h_device_list;
static HWND h_device_name;
static HWND h_device_ip;
static HWND h_device_prefix;
static HWND h_device_protocol;
static HWND h_device_port;
static HWND h_device_continuous;
static HWND h_device_result;
static HWND h_log_list;
static HWND h_status_line;
static HWND h_tools_context_nic;
static HWND h_tools_context_device;
static HWND h_tools_status;
static HWND g_advanced_window;
static HWND h_adv_context_nic;
static HWND h_adv_context_device;
static HWND h_adv_command;
static HWND h_adv_output;
static int g_continuous_ping_active;
static char g_continuous_ping_address[32];
static unsigned long g_ping_sequence;
static unsigned long g_ping_successes;
static unsigned long g_ping_failures;

static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
static LRESULT CALLBACK advanced_window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
static LRESULT CALLBACK advanced_output_subclass_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR subclass_id, DWORD_PTR ref_data);
static void apply_theme(void);
static void update_tools_context(void);
static void open_advanced_tools(void);
static void apply_theme_to_window(HWND hwnd);

static int scale_px(int value) {
    return MulDiv(value, g_dpi, 96);
}

static void get_main_min_track_size(HWND hwnd, POINT *size) {
    RECT rect;
    DWORD style;
    DWORD ex_style;
    BOOL has_menu;
    if (!size) return;
    rect.left = 0;
    rect.top = 0;
    rect.right = scale_px(MAIN_MIN_CLIENT_WIDTH);
    rect.bottom = scale_px(MAIN_MIN_CLIENT_HEIGHT);
    style = hwnd ? (DWORD)GetWindowLongPtrA(hwnd, GWL_STYLE) : WS_OVERLAPPEDWINDOW;
    ex_style = hwnd ? (DWORD)GetWindowLongPtrA(hwnd, GWL_EXSTYLE) : 0;
    has_menu = hwnd ? (GetMenu(hwnd) != NULL) : TRUE;
    if (AdjustWindowRectEx(&rect, style, has_menu, ex_style)) {
        size->x = rect.right - rect.left;
        size->y = rect.bottom - rect.top;
    } else {
        size->x = scale_px(MAIN_MIN_CLIENT_WIDTH);
        size->y = scale_px(MAIN_MIN_CLIENT_HEIGHT);
    }
}

static void set_control_font(HWND control) {
    SendMessage(control, WM_SETFONT, (WPARAM)g_font, TRUE);
}

static HWND make_control(
    DWORD ex_style,
    const char *class_name,
    const char *text,
    DWORD style,
    int x,
    int y,
    int width,
    int height,
    HWND parent,
    int id
) {
    int scaled_x = scale_px(x);
    int scaled_y = scale_px(y);
    int scaled_width = scale_px(width);
    int scaled_height = scale_px(height);
    HWND control = CreateWindowExA(
        ex_style,
        class_name,
        text,
        style | WS_CHILD | WS_VISIBLE,
        scaled_x,
        scaled_y,
        scaled_width,
        scaled_height,
        parent,
        (HMENU)(INT_PTR)id,
        g_instance,
        NULL
    );
    if (control) {
        set_control_font(control);
        ui_layout_add(
            &g_layout,
            control,
            id,
            scaled_x,
            scaled_y,
            scaled_width,
            scaled_height,
            g_register_advanced
        );
    }
    return control;
}

static HWND make_label(const char *text, int x, int y, int width, int height, HWND parent, int id) {
    return make_control(0, "STATIC", text, SS_LEFT | SS_CENTERIMAGE, x, y, width, height, parent, id);
}

static HWND make_button(const char *text, int x, int y, int width, int height, HWND parent, int id) {
    return make_control(0, "BUTTON", text, BS_OWNERDRAW | WS_TABSTOP, x, y, width, height, parent, id);
}

static int measure_group_header_width(const char *title, HWND parent) {
    SIZE size = {0};
    HDC dc = GetDC(parent);
    HGDIOBJ old_font = NULL;
    int logical_width;
    if (!title || !title[0]) return 24;
    if (dc && g_font) old_font = SelectObject(dc, g_font);
    if (!dc || !GetTextExtentPoint32A(dc, title, (int)strlen(title), &size)) {
        size.cx = (LONG)strlen(title) * scale_px(8);
    }
    if (old_font) SelectObject(dc, old_font);
    if (dc) ReleaseDC(parent, dc);
    logical_width = MulDiv(size.cx, 96, g_dpi);
    return logical_width + 12;
}

static HWND make_groupbox(const char *title, int x, int y, int width, int height,
                          HWND parent, int header_id) {
    int header_width = measure_group_header_width(title, parent);
    HWND box = make_control(0, "BUTTON", "", BS_GROUPBOX, x, y, width, height, parent, 0);
    HWND label = make_control(0, "STATIC", title, SS_CENTER | SS_CENTERIMAGE,
                              x + 8, y - 1, header_width, 18, parent, header_id);
    if (label) SetPropA(label, "NIC_GROUP_HEADER", (HANDLE)1);
    return box;
}

static HWND make_edit(const char *text, int x, int y, int width, int height, HWND parent, int id) {
    return make_control(
        WS_EX_CLIENTEDGE,
        "EDIT",
        text,
        ES_AUTOHSCROLL | WS_TABSTOP,
        x,
        y,
        width,
        height,
        parent,
        id
    );
}

static void get_text(HWND control, char *buffer, int size) {
    if (size <= 0) return;
    buffer[0] = '\0';
    if (!control) return;
    GetWindowTextA(control, buffer, size);
    buffer[size - 1] = '\0';
}

static void set_text(HWND control, const char *value) {
    SetWindowTextA(control, value ? value : "");
}

static void trim(char *value) {
    string_trim(value);
}

static void get_app_directory(char *directory, size_t size) {
    char *slash;
    GetModuleFileNameA(NULL, directory, (DWORD)size);
    directory[size - 1] = '\0';
    slash = strrchr(directory, '\\');
    if (slash) *slash = '\0';
}

static void build_app_path(char *output, size_t output_size, const char *filename) {
    char directory[MAX_PATH];
    get_app_directory(directory, sizeof(directory));
    snprintf(output, output_size, "%s\\%s", directory, filename);
    output[output_size - 1] = '\0';
}

static void append_log(const char *format, ...) {
    char message[1024];
    char line[1200];
    char timestamp[64];
    time_t now;
    struct tm local_time;
    va_list args;
    FILE *file;
    int item_count;

    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    message[sizeof(message) - 1] = '\0';

    now = time(NULL);
    localtime_s(&local_time, &now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &local_time);
    snprintf(line, sizeof(line), "[%s] %s", timestamp, message);
    line[sizeof(line) - 1] = '\0';

    if (h_log_list) {
        SendMessageA(h_log_list, LB_ADDSTRING, 0, (LPARAM)line);
        item_count = (int)SendMessage(h_log_list, LB_GETCOUNT, 0, 0);
        if (item_count > 500) SendMessage(h_log_list, LB_DELETESTRING, 0, 0);
        SendMessage(h_log_list, LB_SETTOPINDEX, (WPARAM)(item_count - 1), 0);
    }
    if (h_status_line) set_text(h_status_line, message);

    file = fopen(g_log_path, "ab");
    if (file) {
        fprintf(file, "%s\r\n", line);
        fclose(file);
    }
}

static int is_valid_ipv4(const char *value) {
    return ipv4_is_valid(value);
}

static int parse_prefix(const char *value, int *prefix_out) {
    return ipv4_parse_prefix(value, prefix_out);
}

static void wide_to_ansi(const WCHAR *source, char *destination, int size) {
    if (!source || !source[0]) {
        destination[0] = '\0';
        return;
    }
    WideCharToMultiByte(CP_ACP, 0, source, -1, destination, size, NULL, NULL);
    destination[size - 1] = '\0';
}

static void sockaddr_to_string(const SOCKADDR *address, char *output, int size) {
    const SOCKADDR_IN *ipv4;
    if (!address || address->sa_family != AF_INET) {
        output[0] = '\0';
        return;
    }
    ipv4 = (const SOCKADDR_IN *)address;
    if (!InetNtopA(AF_INET, (PVOID)&ipv4->sin_addr, output, (DWORD)size)) output[0] = '\0';
}

static int enumerate_adapters(void) {
    ULONG flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_INCLUDE_GATEWAYS;
    ULONG buffer_size = 16 * 1024;
    ULONG result;
    IP_ADAPTER_ADDRESSES *buffer;
    IP_ADAPTER_ADDRESSES *item;

    g_adapter_count = 0;
    buffer = (IP_ADAPTER_ADDRESSES *)malloc(buffer_size);
    if (!buffer) return 0;
    result = GetAdaptersAddresses(AF_INET, flags, NULL, buffer, &buffer_size);
    if (result == ERROR_BUFFER_OVERFLOW) {
        free(buffer);
        buffer = (IP_ADAPTER_ADDRESSES *)malloc(buffer_size);
        if (!buffer) return 0;
        result = GetAdaptersAddresses(AF_INET, flags, NULL, buffer, &buffer_size);
    }
    if (result != NO_ERROR) {
        free(buffer);
        return 0;
    }

    for (item = buffer; item && g_adapter_count < MAX_ADAPTERS; item = item->Next) {
        AdapterInfo *adapter;
        IP_ADAPTER_UNICAST_ADDRESS *unicast;
        IP_ADAPTER_DNS_SERVER_ADDRESS *dns;
        int dns_count = 0;

        if (item->IfIndex == 0) continue;
        if (item->IfType != IF_TYPE_ETHERNET_CSMACD &&
            item->IfType != IF_TYPE_IEEE80211 &&
            item->IfType != IF_TYPE_PPP) continue;

        adapter = &g_adapters[g_adapter_count];
        memset(adapter, 0, sizeof(*adapter));
        wide_to_ansi(item->FriendlyName, adapter->name, sizeof(adapter->name));
        wide_to_ansi(item->Description, adapter->description, sizeof(adapter->description));
        strncpy(adapter->adapter_id, item->AdapterName ? item->AdapterName : "", sizeof(adapter->adapter_id) - 1);
        adapter->if_index = item->IfIndex;
        adapter->operational_status = item->OperStatus;
        adapter->link_speed = item->TransmitLinkSpeed;
        adapter->dhcp_enabled = item->Dhcpv4Enabled ? 1 : 0;

        for (unicast = item->FirstUnicastAddress;
             unicast && adapter->ip_count < MAX_ADAPTER_IPS;
             unicast = unicast->Next) {
            if (!unicast->Address.lpSockaddr || unicast->Address.lpSockaddr->sa_family != AF_INET) continue;
            sockaddr_to_string(
                unicast->Address.lpSockaddr,
                adapter->ip[adapter->ip_count],
                sizeof(adapter->ip[adapter->ip_count])
            );
            adapter->prefix[adapter->ip_count] = unicast->OnLinkPrefixLength;
            if (adapter->ip[adapter->ip_count][0]) ++adapter->ip_count;
        }
        if (item->FirstGatewayAddress && item->FirstGatewayAddress->Address.lpSockaddr) {
            sockaddr_to_string(
                item->FirstGatewayAddress->Address.lpSockaddr,
                adapter->gateway,
                sizeof(adapter->gateway)
            );
        }
        for (dns = item->FirstDnsServerAddress; dns && dns_count < 2; dns = dns->Next) {
            char value[32];
            if (!dns->Address.lpSockaddr || dns->Address.lpSockaddr->sa_family != AF_INET) continue;
            sockaddr_to_string(dns->Address.lpSockaddr, value, sizeof(value));
            if (!value[0]) continue;
            if (dns_count == 0) snprintf(adapter->dns1, sizeof(adapter->dns1), "%s", value);
            else snprintf(adapter->dns2, sizeof(adapter->dns2), "%s", value);
            ++dns_count;
        }
        ++g_adapter_count;
    }
    free(buffer);
    return 1;
}

static AdapterInfo *selected_adapter(void) {
    int selection = (int)SendMessage(h_adapter_combo, CB_GETCURSEL, 0, 0);
    if (selection < 0 || selection >= g_adapter_count) return NULL;
    return &g_adapters[selection];
}

static void update_adapter_summary(void) {
    AdapterInfo *adapter = selected_adapter();
    char buffer[1024];
    char speed[64];
    int i;
    if (!adapter) {
        set_text(h_adapter_status, "Status: No compatible NIC selected");
        set_text(h_adapter_link, "Link speed: --");
        set_text(h_adapter_desc, "Description: --");
        set_text(h_adapter_addrs, "IPv4: --");
        update_tools_context();
        return;
    }
    snprintf(
        buffer,
        sizeof(buffer),
        "Status: %s",
        adapter->operational_status == IfOperStatusUp ? "Connected" : "Disconnected"
    );
    set_text(h_adapter_status, buffer);
    diagnostics_format_link_speed(adapter->operational_status == IfOperStatusUp, adapter->link_speed, speed, sizeof(speed));
    snprintf(buffer, sizeof(buffer), "Link speed: %s", speed);
    set_text(h_adapter_link, buffer);
    snprintf(buffer, sizeof(buffer), "Description: %s  |  Interface index: %lu", adapter->description, adapter->if_index);
    set_text(h_adapter_desc, buffer);

    strcpy(buffer, "IPv4: ");
    if (adapter->ip_count == 0) {
        strcat(buffer, "None");
    } else {
        for (i = 0; i < adapter->ip_count; ++i) {
            char entry[64];
            snprintf(entry, sizeof(entry), "%s%s/%d", i ? ", " : "", adapter->ip[i], adapter->prefix[i]);
            if (strlen(buffer) + strlen(entry) + 1 < sizeof(buffer)) strcat(buffer, entry);
        }
    }
    set_text(h_adapter_addrs, buffer);
    update_tools_context();
}

static void toggle_static_controls(void) {
    int enabled = SendMessage(h_dhcp, BM_GETCHECK, 0, 0) != BST_CHECKED;
    EnableWindow(h_primary_ip, enabled);
    EnableWindow(h_prefix, enabled);
    EnableWindow(h_gateway, enabled);
    EnableWindow(h_dns1, enabled);
    EnableWindow(h_dns2, enabled);
    EnableWindow(h_secondary_list, enabled);
    EnableWindow(h_secondary_ip, enabled);
    EnableWindow(h_secondary_prefix, enabled);
    EnableWindow(GetDlgItem(g_main, ID_SECONDARY_ADD), enabled);
    EnableWindow(GetDlgItem(g_main, ID_SECONDARY_REMOVE), enabled);
}

static void load_adapter_into_form(void) {
    AdapterInfo *adapter = selected_adapter();
    char prefix[16];
    int i;
    if (!adapter) return;
    SendMessage(h_dhcp, BM_SETCHECK, adapter->dhcp_enabled ? BST_CHECKED : BST_UNCHECKED, 0);
    if (adapter->ip_count > 0) {
        set_text(h_primary_ip, adapter->ip[0]);
        snprintf(prefix, sizeof(prefix), "%d", adapter->prefix[0]);
        set_text(h_prefix, prefix);
    } else {
        set_text(h_primary_ip, "");
        set_text(h_prefix, "24");
    }
    set_text(h_gateway, adapter->gateway);
    set_text(h_dns1, adapter->dns1);
    set_text(h_dns2, adapter->dns2);
    SendMessage(h_secondary_list, LB_RESETCONTENT, 0, 0);
    for (i = 1; i < adapter->ip_count; ++i) {
        char secondary[64];
        snprintf(secondary, sizeof(secondary), "%s/%d", adapter->ip[i], adapter->prefix[i]);
        SendMessageA(h_secondary_list, LB_ADDSTRING, 0, (LPARAM)secondary);
    }
    toggle_static_controls();
}

static void refresh_adapters(int load_form) {
    char previous_id[128] = "";
    AdapterInfo *previous = selected_adapter();
    int selected = 0;
    int i;
    if (previous) strncpy(previous_id, previous->adapter_id, sizeof(previous_id) - 1);
    if (!enumerate_adapters()) {
        append_log("Unable to enumerate network adapters (error %lu).", GetLastError());
        return;
    }
    SendMessage(h_adapter_combo, CB_RESETCONTENT, 0, 0);
    for (i = 0; i < g_adapter_count; ++i) {
        char display[320];
        snprintf(display, sizeof(display), "%s%s", g_adapters[i].name,
                 g_adapters[i].operational_status == IfOperStatusUp ? "  (Connected)" : "  (Disconnected)");
        SendMessageA(h_adapter_combo, CB_ADDSTRING, 0, (LPARAM)display);
        if (previous_id[0] && strcmp(previous_id, g_adapters[i].adapter_id) == 0) selected = i;
    }
    if (g_adapter_count > 0) SendMessage(h_adapter_combo, CB_SETCURSEL, selected, 0);
    update_adapter_summary();
    if (load_form) load_adapter_into_form();
}

static void populate_profile_list(void) {
    int i;
    SendMessage(h_profile_combo, CB_RESETCONTENT, 0, 0);
    for (i = 0; i < g_config.profile_count; ++i) {
        SendMessageA(h_profile_combo, CB_ADDSTRING, 0, (LPARAM)g_config.profiles[i].name);
    }
}

static void populate_device_list(void) {
    int i;
    SendMessage(h_device_list, LB_RESETCONTENT, 0, 0);
    for (i = 0; i < g_config.device_count; ++i) {
        const DeviceEntry *device = &g_config.devices[i];
        char display[320];
        if (device->port > 0) {
            snprintf(display, sizeof(display), "%s  -  %s/%d  -  %s://%s:%d",
                     device->name, device->address, device->prefix,
                     device->protocol, device->address, device->port);
        } else {
            snprintf(display, sizeof(display), "%s  -  %s/%d  -  %s://%s",
                     device->name, device->address, device->prefix,
                     device->protocol, device->address);
        }
        SendMessageA(h_device_list, LB_ADDSTRING, 0, (LPARAM)display);
    }
}

static int save_config(void) {
    if (!config_save(g_config_path, &g_config)) {
        MessageBoxA(g_main, "The portable configuration file could not be saved. Make sure the application folder is writable.",
                    APP_TITLE, MB_OK | MB_ICONERROR);
        append_log("Configuration save failed: %s", g_config_path);
        return 0;
    }
    return 1;
}

static int windows_uses_dark_theme(void) {
    DWORD light_theme = 1;
    DWORD size = sizeof(light_theme);
    LONG result = RegGetValueA(
        HKEY_CURRENT_USER,
        "Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        "AppsUseLightTheme",
        RRF_RT_REG_DWORD,
        NULL,
        &light_theme,
        &size
    );
    if (result != ERROR_SUCCESS) return 0;
    return light_theme == 0;
}

static void set_theme_palette(int dark) {
    HBRUSH old_background = g_background_brush;
    HBRUSH old_edit = g_edit_brush;
    HBRUSH old_button = g_button_brush;
    HBRUSH old_pressed = g_button_pressed_brush;

    g_dark_theme = dark;
    if (dark) {
        g_background_color = RGB(28, 31, 36);
        g_edit_color = RGB(39, 43, 50);
        g_button_color = RGB(52, 58, 68);
        g_button_pressed_color = RGB(66, 75, 89);
        g_text_color = RGB(235, 238, 242);
        g_muted_text_color = RGB(164, 171, 182);
        g_border_color = RGB(91, 99, 112);
    } else {
        g_background_color = RGB(245, 246, 248);
        g_edit_color = RGB(255, 255, 255);
        g_button_color = RGB(232, 235, 239);
        g_button_pressed_color = RGB(210, 218, 228);
        g_text_color = RGB(25, 29, 35);
        g_muted_text_color = RGB(92, 99, 110);
        g_border_color = RGB(145, 151, 160);
    }
    g_background_brush = CreateSolidBrush(g_background_color);
    g_edit_brush = CreateSolidBrush(g_edit_color);
    g_button_brush = CreateSolidBrush(g_button_color);
    g_button_pressed_brush = CreateSolidBrush(g_button_pressed_color);

    if (g_main) SetClassLongPtrA(g_main, GCLP_HBRBACKGROUND, (LONG_PTR)g_background_brush);
    if (old_background) DeleteObject(old_background);
    if (old_edit) DeleteObject(old_edit);
    if (old_button) DeleteObject(old_button);
    if (old_pressed) DeleteObject(old_pressed);
}

static BOOL CALLBACK apply_theme_to_child(HWND child, LPARAM parameter) {
    (void)parameter;
    SetWindowTheme(child, L"", L"");
    InvalidateRect(child, NULL, TRUE);
    return TRUE;
}

static void apply_theme(void) {
    BOOL dark_title;
    HRESULT title_result;
    int dark;
    int selection;

    if (_stricmp(g_config.theme, "dark") == 0) dark = 1;
    else if (_stricmp(g_config.theme, "light") == 0) dark = 0;
    else dark = windows_uses_dark_theme();

    set_theme_palette(dark);
    if (h_theme_combo) {
        if (_stricmp(g_config.theme, "light") == 0) selection = 1;
        else if (_stricmp(g_config.theme, "dark") == 0) selection = 2;
        else selection = 0;
        SendMessage(h_theme_combo, CB_SETCURSEL, selection, 0);
    }

    if (!g_main) return;
    dark_title = dark ? TRUE : FALSE;
    title_result = DwmSetWindowAttribute(g_main, 20, &dark_title, sizeof(dark_title));
    if (FAILED(title_result)) {
        DwmSetWindowAttribute(g_main, 19, &dark_title, sizeof(dark_title));
    }
    SetWindowTheme(g_main, L"", L"");
    EnumChildWindows(g_main, apply_theme_to_child, 0);
    RedrawWindow(
        g_main,
        NULL,
        NULL,
        RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_FRAME
    );
    if (g_advanced_window && IsWindow(g_advanced_window)) apply_theme_to_window(g_advanced_window);
}

static void draw_owner_button(const DRAWITEMSTRUCT *item) {
    RECT rect = item->rcItem;
    HBRUSH brush;
    char text[256];
    COLORREF color;
    UINT text_flags = DT_CENTER | DT_VCENTER | DT_SINGLELINE;

    brush = (item->itemState & ODS_SELECTED) ? g_button_pressed_brush : g_button_brush;
    FillRect(item->hDC, &rect, brush);
    FrameRect(item->hDC, &rect, (HBRUSH)GetStockObject(GRAY_BRUSH));
    if (item->itemState & ODS_SELECTED) OffsetRect(&rect, 1, 1);

    GetWindowTextA(item->hwndItem, text, sizeof(text));
    color = (item->itemState & ODS_DISABLED) ? g_muted_text_color : g_text_color;
    SetTextColor(item->hDC, color);
    SetBkMode(item->hDC, TRANSPARENT);
    SelectObject(item->hDC, g_font);
    DrawTextA(item->hDC, text, -1, &rect, text_flags);
    if (item->itemState & ODS_FOCUS) {
        InflateRect(&rect, -3, -3);
        DrawFocusRect(item->hDC, &rect);
    }
}

static void draw_owner_combo(const DRAWITEMSTRUCT *item) {
    RECT rect = item->rcItem;
    char text[512] = "";
    int item_index = (int)item->itemID;
    HBRUSH brush;

    if (item_index < 0) item_index = (int)SendMessage(item->hwndItem, CB_GETCURSEL, 0, 0);
    if (item_index >= 0) {
        SendMessageA(item->hwndItem, CB_GETLBTEXT, item_index, (LPARAM)text);
    }
    brush = (item->itemState & ODS_SELECTED) ? g_button_pressed_brush : g_edit_brush;
    FillRect(item->hDC, &rect, brush);
    SetTextColor(item->hDC, g_text_color);
    SetBkMode(item->hDC, TRANSPARENT);
    SelectObject(item->hDC, g_font);
    rect.left += 5;
    DrawTextA(item->hDC, text, -1, &rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    if (item->itemState & ODS_FOCUS) DrawFocusRect(item->hDC, &item->rcItem);
}

static void append_command(char *command, size_t size, const char *format, ...) {
    size_t used = strlen(command);
    va_list args;
    if (used >= size - 1) return;
    va_start(args, format);
    vsnprintf(command + used, size - used, format, args);
    va_end(args);
    command[size - 1] = '\0';
}

static int run_powershell(const char *script, char *output, size_t output_size, DWORD *exit_code) {
    SECURITY_ATTRIBUTES security;
    HANDLE read_pipe = NULL;
    HANDLE write_pipe = NULL;
    STARTUPINFOA startup;
    PROCESS_INFORMATION process;
    char command_line[16384];
    DWORD wait_result;
    DWORD bytes_read;
    size_t total = 0;
    int success = 0;

    if (output_size > 0) output[0] = '\0';
    memset(&security, 0, sizeof(security));
    security.nLength = sizeof(security);
    security.bInheritHandle = TRUE;
    if (!CreatePipe(&read_pipe, &write_pipe, &security, 0)) return 0;
    SetHandleInformation(read_pipe, HANDLE_FLAG_INHERIT, 0);

    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    startup.wShowWindow = SW_HIDE;
    startup.hStdOutput = write_pipe;
    startup.hStdError = write_pipe;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    memset(&process, 0, sizeof(process));

    snprintf(
        command_line,
        sizeof(command_line),
        "powershell.exe -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -Command \"%s\"",
        script
    );
    command_line[sizeof(command_line) - 1] = '\0';

    if (!CreateProcessA(NULL, command_line, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &startup, &process)) {
        CloseHandle(read_pipe);
        CloseHandle(write_pipe);
        return 0;
    }
    CloseHandle(write_pipe);
    write_pipe = NULL;

    wait_result = WaitForSingleObject(process.hProcess, 60000);
    if (wait_result == WAIT_TIMEOUT) {
        TerminateProcess(process.hProcess, ERROR_TIMEOUT);
        if (output_size > 0) strncpy(output, "The network command timed out.", output_size - 1);
    }
    GetExitCodeProcess(process.hProcess, exit_code);

    while (total + 1 < output_size &&
           ReadFile(read_pipe, output + total, (DWORD)(output_size - total - 1), &bytes_read, NULL) &&
           bytes_read > 0) {
        total += bytes_read;
    }
    if (output_size > 0) output[total] = '\0';
    success = wait_result == WAIT_OBJECT_0;

    CloseHandle(read_pipe);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return success;
}

static int collect_profile_from_form(NetworkProfile *profile, int require_name) {
    char prefix_text[32];
    int item_count;
    int i;
    AdapterInfo *adapter = selected_adapter();

    memset(profile, 0, sizeof(*profile));
    get_text(h_profile_name, profile->name, sizeof(profile->name));
    trim(profile->name);
    if (require_name && !profile->name[0]) {
        MessageBoxA(g_main, "Enter a profile name first.", APP_TITLE, MB_OK | MB_ICONWARNING);
        return 0;
    }
    if (adapter) snprintf(profile->adapter_id, sizeof(profile->adapter_id), "%s", adapter->adapter_id);
    profile->dhcp = SendMessage(h_dhcp, BM_GETCHECK, 0, 0) == BST_CHECKED;
    get_text(h_primary_ip, profile->primary_ip, sizeof(profile->primary_ip));
    get_text(h_prefix, prefix_text, sizeof(prefix_text));
    get_text(h_gateway, profile->gateway, sizeof(profile->gateway));
    get_text(h_dns1, profile->dns1, sizeof(profile->dns1));
    get_text(h_dns2, profile->dns2, sizeof(profile->dns2));
    trim(profile->primary_ip);
    trim(prefix_text);
    trim(profile->gateway);
    trim(profile->dns1);
    trim(profile->dns2);

    if (!profile->dhcp) {
        if (!is_valid_ipv4(profile->primary_ip)) {
            MessageBoxA(g_main, "Enter a valid primary IPv4 address.", APP_TITLE, MB_OK | MB_ICONWARNING);
            return 0;
        }
        if (!parse_prefix(prefix_text, &profile->prefix)) {
            MessageBoxA(g_main, "Enter a prefix from 1-32 or a contiguous subnet mask.", APP_TITLE, MB_OK | MB_ICONWARNING);
            return 0;
        }
        if (profile->gateway[0] && !is_valid_ipv4(profile->gateway)) {
            MessageBoxA(g_main, "The default gateway is not a valid IPv4 address.", APP_TITLE, MB_OK | MB_ICONWARNING);
            return 0;
        }
        if (profile->dns1[0] && !is_valid_ipv4(profile->dns1)) {
            MessageBoxA(g_main, "DNS server 1 is not a valid IPv4 address.", APP_TITLE, MB_OK | MB_ICONWARNING);
            return 0;
        }
        if (profile->dns2[0] && !is_valid_ipv4(profile->dns2)) {
            MessageBoxA(g_main, "DNS server 2 is not a valid IPv4 address.", APP_TITLE, MB_OK | MB_ICONWARNING);
            return 0;
        }
    } else {
        profile->prefix = 24;
    }

    item_count = (int)SendMessage(h_secondary_list, LB_GETCOUNT, 0, 0);
    for (i = 0; i < item_count && i < MAX_SECONDARY; ++i) {
        char combined[64];
        char *slash;
        SendMessageA(h_secondary_list, LB_GETTEXT, i, (LPARAM)combined);
        slash = strrchr(combined, '/');
        if (!slash) continue;
        *slash = '\0';
        if (!is_valid_ipv4(combined)) continue;
        snprintf(profile->secondary_ip[profile->secondary_count], 32, "%s", combined);
        profile->secondary_prefix[profile->secondary_count] = atoi(slash + 1);
        ++profile->secondary_count;
    }
    return 1;
}

static void load_profile_into_form(const NetworkProfile *profile) {
    char number[16];
    int i;
    set_text(h_profile_name, profile->name);
    SendMessage(h_dhcp, BM_SETCHECK, profile->dhcp ? BST_CHECKED : BST_UNCHECKED, 0);
    set_text(h_primary_ip, profile->primary_ip);
    snprintf(number, sizeof(number), "%d", profile->prefix ? profile->prefix : 24);
    set_text(h_prefix, number);
    set_text(h_gateway, profile->gateway);
    set_text(h_dns1, profile->dns1);
    set_text(h_dns2, profile->dns2);
    SendMessage(h_secondary_list, LB_RESETCONTENT, 0, 0);
    for (i = 0; i < profile->secondary_count; ++i) {
        char combined[64];
        snprintf(combined, sizeof(combined), "%s/%d", profile->secondary_ip[i], profile->secondary_prefix[i]);
        SendMessageA(h_secondary_list, LB_ADDSTRING, 0, (LPARAM)combined);
    }
    toggle_static_controls();
}

static void save_profile(void) {
    NetworkProfile profile;
    int i;
    int found = -1;
    if (!collect_profile_from_form(&profile, 1)) return;
    for (i = 0; i < g_config.profile_count; ++i) {
        if (_stricmp(g_config.profiles[i].name, profile.name) == 0) {
            found = i;
            break;
        }
    }
    if (found >= 0) {
        g_config.profiles[found] = profile;
    } else {
        if (g_config.profile_count >= MAX_PROFILES) {
            MessageBoxA(g_main, "The profile limit has been reached.", APP_TITLE, MB_OK | MB_ICONWARNING);
            return;
        }
        found = g_config.profile_count++;
        g_config.profiles[found] = profile;
    }
    if (save_config()) {
        populate_profile_list();
        SendMessage(h_profile_combo, CB_SETCURSEL, found, 0);
        append_log("Saved network profile \"%s\".", profile.name);
    }
}

static void delete_profile(void) {
    int selection = (int)SendMessage(h_profile_combo, CB_GETCURSEL, 0, 0);
    int i;
    char name[128];
    if (selection < 0 || selection >= g_config.profile_count) return;
    strncpy(name, g_config.profiles[selection].name, sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';
    if (MessageBoxA(g_main, "Delete the selected network profile?", APP_TITLE,
                    MB_YESNO | MB_ICONQUESTION) != IDYES) return;
    for (i = selection; i + 1 < g_config.profile_count; ++i) {
        g_config.profiles[i] = g_config.profiles[i + 1];
    }
    --g_config.profile_count;
    if (save_config()) {
        populate_profile_list();
        set_text(h_profile_name, "");
        append_log("Deleted network profile \"%s\".", name);
    }
}

static void save_network_backup(const AdapterInfo *adapter) {
    FILE *file = fopen(g_backup_path, "wb");
    int i;
    time_t now;
    if (!file) {
        append_log("Warning: unable to write the last-known network backup.");
        return;
    }
    now = time(NULL);
    fprintf(file, "NIC Equipment Manager - last configuration backup\r\n");
    fprintf(file, "Created: %s", ctime(&now));
    fprintf(file, "Adapter: %s\r\nDescription: %s\r\nInterface index: %lu\r\n",
            adapter->name, adapter->description, adapter->if_index);
    fprintf(file, "DHCP: %s\r\n", adapter->dhcp_enabled ? "Enabled" : "Disabled");
    for (i = 0; i < adapter->ip_count; ++i) {
        fprintf(file, "IPv4 %d: %s/%d\r\n", i + 1, adapter->ip[i], adapter->prefix[i]);
    }
    fprintf(file, "Gateway: %s\r\nDNS 1: %s\r\nDNS 2: %s\r\n",
            adapter->gateway, adapter->dns1, adapter->dns2);
    fclose(file);
}

static int another_active_gateway_exists(ULONG selected_if_index) {
    int i;
    for (i = 0; i < g_adapter_count; ++i) {
        if (g_adapters[i].if_index != selected_if_index &&
            g_adapters[i].operational_status == IfOperStatusUp &&
            g_adapters[i].gateway[0]) return 1;
    }
    return 0;
}

static void apply_network_settings(void) {
    AdapterInfo *adapter = selected_adapter();
    NetworkProfile settings;
    char command[14000] = "";
    char output[8192];
    DWORD exit_code = 1;
    int i;
    int confirmation;

    if (!adapter) {
        MessageBoxA(g_main, "Select a network adapter first.", APP_TITLE, MB_OK | MB_ICONWARNING);
        return;
    }
    if (!collect_profile_from_form(&settings, 0)) return;
    if (!settings.dhcp && settings.gateway[0] && another_active_gateway_exists(adapter->if_index)) {
        confirmation = MessageBoxA(
            g_main,
            "Another connected adapter already has a default gateway. Multiple gateways can cause unexpected routing.\r\n\r\nApply this gateway anyway?",
            APP_TITLE,
            MB_YESNO | MB_ICONWARNING
        );
        if (confirmation != IDYES) return;
    }
    {
        char prompt[512];
        snprintf(
            prompt,
            sizeof(prompt),
            "Apply %s configuration to:\r\n\r\n%s\r\n\r\nThe adapter may disconnect temporarily. Continue?",
            settings.dhcp ? "DHCP" : "static IPv4",
            adapter->name
        );
        if (MessageBoxA(g_main, prompt, APP_TITLE, MB_YESNO | MB_ICONWARNING) != IDYES) return;
    }

    save_network_backup(adapter);
    append_command(command, sizeof(command), "$ErrorActionPreference='Stop'; $i=%lu; ", adapter->if_index);
    if (settings.dhcp) {
        {
            char persistent_dhcp[2048];
            if (!network_build_persistent_dhcp(persistent_dhcp, sizeof(persistent_dhcp), 1)) {
                MessageBoxA(g_main, "Unable to prepare the persistent DHCP command.", APP_TITLE, MB_OK | MB_ICONERROR);
                return;
            }
            append_command(command, sizeof(command), "%s", persistent_dhcp);
        }
        append_command(
            command,
            sizeof(command),
            "Set-NetIPInterface -InterfaceIndex $i -AddressFamily IPv4 -Dhcp Enabled -ErrorAction Stop; "
            "Get-NetIPAddress -InterfaceIndex $i -AddressFamily IPv4 -ErrorAction SilentlyContinue | "
            "Where-Object {$_.PrefixOrigin -eq 'Manual'} | "
            "Remove-NetIPAddress -Confirm:$false -ErrorAction SilentlyContinue; "
            "Set-DnsClientServerAddress -InterfaceIndex $i -ResetServerAddresses -ErrorAction Stop; "
        );
        {
            char verify_dhcp[2048];
            if (!network_build_dhcp_verification(verify_dhcp, sizeof(verify_dhcp))) {
                MessageBoxA(g_main, "Unable to prepare DHCP verification.", APP_TITLE, MB_OK | MB_ICONERROR);
                return;
            }
            append_command(command, sizeof(command), "%s", verify_dhcp);
        }
        append_command(command, sizeof(command), "Write-Output 'DHCP configuration applied and verified.'");
    } else {
        {
            char persistent_dhcp[2048];
            if (!network_build_persistent_dhcp(persistent_dhcp, sizeof(persistent_dhcp), 0)) {
                MessageBoxA(g_main, "Unable to prepare the persistent static-IP command.", APP_TITLE, MB_OK | MB_ICONERROR);
                return;
            }
            append_command(command, sizeof(command), "%s", persistent_dhcp);
        }
        {
            char static_reset[4096];
            if (!network_build_static_reset_preamble(static_reset, sizeof(static_reset))) {
                MessageBoxA(g_main, "Unable to prepare the static IPv4 reset command.", APP_TITLE, MB_OK | MB_ICONERROR);
                return;
            }
            append_command(command, sizeof(command), "%s", static_reset);
        }
        {
            char static_primary[2048];
            if (!network_build_netsh_static_primary(
                    static_primary,
                    sizeof(static_primary),
                    settings.primary_ip,
                    settings.prefix,
                    settings.gateway)) {
                MessageBoxA(g_main, "Unable to prepare the static IPv4 command.", APP_TITLE, MB_OK | MB_ICONERROR);
                return;
            }
            append_command(command, sizeof(command), "%s", static_primary);
        }
        for (i = 0; i < settings.secondary_count; ++i) {
            char static_secondary[1024];
            if (!network_build_netsh_static_secondary(
                    static_secondary,
                    sizeof(static_secondary),
                    settings.secondary_ip[i],
                    settings.secondary_prefix[i])) {
                MessageBoxA(g_main, "Unable to prepare a secondary IPv4 command.", APP_TITLE, MB_OK | MB_ICONERROR);
                return;
            }
            append_command(command, sizeof(command), "%s", static_secondary);
        }
        if (settings.dns1[0] && settings.dns2[0]) {
            append_command(
                command,
                sizeof(command),
                "Set-DnsClientServerAddress -InterfaceIndex $i -ServerAddresses @('%s','%s') -ErrorAction Stop; ",
                settings.dns1,
                settings.dns2
            );
        } else if (settings.dns1[0]) {
            append_command(
                command,
                sizeof(command),
                "Set-DnsClientServerAddress -InterfaceIndex $i -ServerAddresses @('%s') -ErrorAction Stop; ",
                settings.dns1
            );
        } else {
            append_command(
                command,
                sizeof(command),
                "Set-DnsClientServerAddress -InterfaceIndex $i -ResetServerAddresses -ErrorAction Stop; "
            );
        }
        {
            char verify_static[2048];
            if (!network_build_static_verification(
                    verify_static,
                    sizeof(verify_static),
                    settings.primary_ip,
                    settings.prefix)) {
                MessageBoxA(g_main, "Unable to prepare static IPv4 verification.", APP_TITLE, MB_OK | MB_ICONERROR);
                return;
            }
            append_command(command, sizeof(command), "%s", verify_static);
        }
        append_command(command, sizeof(command), "Write-Output 'Static IPv4 configuration applied and verified.'");
    }

    SetCursor(LoadCursor(NULL, IDC_WAIT));
    EnableWindow(GetDlgItem(g_main, ID_APPLY_CURRENT), FALSE);
    append_log("Applying network settings to %s...", adapter->name);
    if (!run_powershell(command, output, sizeof(output), &exit_code)) {
        MessageBoxA(g_main, "Windows could not start or complete the network command.", APP_TITLE, MB_OK | MB_ICONERROR);
        append_log("Network command could not be completed.");
    } else if (exit_code != 0) {
        char message[8500];
        snprintf(message, sizeof(message), "Windows reported an error while applying the configuration:\r\n\r\n%s", output);
        MessageBoxA(g_main, message, APP_TITLE, MB_OK | MB_ICONERROR);
        append_log("Network configuration failed (exit code %lu): %.500s", exit_code, output);
    } else {
        append_log("%s", output[0] ? output : "Network configuration applied successfully.");
        MessageBoxA(g_main, "The IPv4 configuration was applied successfully.", APP_TITLE, MB_OK | MB_ICONINFORMATION);
    }
    EnableWindow(GetDlgItem(g_main, ID_APPLY_CURRENT), TRUE);
    SetCursor(LoadCursor(NULL, IDC_ARROW));
    Sleep(750);
    refresh_adapters(1);
}

static void add_secondary_address(void) {
    char ip[32];
    char prefix_text[32];
    char combined[64];
    int prefix;
    int count;
    int i;
    get_text(h_secondary_ip, ip, sizeof(ip));
    get_text(h_secondary_prefix, prefix_text, sizeof(prefix_text));
    trim(ip);
    trim(prefix_text);
    if (!is_valid_ipv4(ip) || !parse_prefix(prefix_text, &prefix)) {
        MessageBoxA(g_main, "Enter a valid secondary IPv4 address and prefix/subnet mask.",
                    APP_TITLE, MB_OK | MB_ICONWARNING);
        return;
    }
    snprintf(combined, sizeof(combined), "%s/%d", ip, prefix);
    count = (int)SendMessage(h_secondary_list, LB_GETCOUNT, 0, 0);
    if (count >= MAX_SECONDARY) {
        MessageBoxA(g_main, "The secondary address limit has been reached.", APP_TITLE, MB_OK | MB_ICONWARNING);
        return;
    }
    for (i = 0; i < count; ++i) {
        char existing[64];
        SendMessageA(h_secondary_list, LB_GETTEXT, i, (LPARAM)existing);
        if (_stricmp(existing, combined) == 0) return;
    }
    SendMessageA(h_secondary_list, LB_ADDSTRING, 0, (LPARAM)combined);
    set_text(h_secondary_ip, "");
}

static void remove_secondary_address(void) {
    int selection = (int)SendMessage(h_secondary_list, LB_GETCURSEL, 0, 0);
    if (selection >= 0) SendMessage(h_secondary_list, LB_DELETESTRING, selection, 0);
}

static int get_device_fields(DeviceEntry *device, int require_name) {
    char port_text[16];
    char prefix_text[32];
    int protocol_selection;
    char *end;
    long port;
    memset(device, 0, sizeof(*device));
    get_text(h_device_name, device->name, sizeof(device->name));
    get_text(h_device_ip, device->address, sizeof(device->address));
    get_text(h_device_prefix, prefix_text, sizeof(prefix_text));
    get_text(h_device_port, port_text, sizeof(port_text));
    trim(device->name);
    trim(device->address);
    trim(prefix_text);
    trim(port_text);
    if (require_name && !device->name[0]) {
        MessageBoxA(g_main, "Enter a friendly device name before saving.", APP_TITLE, MB_OK | MB_ICONWARNING);
        return 0;
    }
    if (!is_valid_ipv4(device->address)) {
        MessageBoxA(g_main, "Enter a valid device IPv4 address.", APP_TITLE, MB_OK | MB_ICONWARNING);
        return 0;
    }
    if (!parse_prefix(prefix_text, &device->prefix)) {
        MessageBoxA(g_main, "Enter a device prefix from 1-32 or a contiguous subnet mask.",
                    APP_TITLE, MB_OK | MB_ICONWARNING);
        return 0;
    }
    protocol_selection = (int)SendMessage(h_device_protocol, CB_GETCURSEL, 0, 0);
    strcpy(device->protocol, protocol_selection == 1 ? "https" : "http");
    if (port_text[0]) {
        port = strtol(port_text, &end, 10);
        if (*end || port < 1 || port > 65535) {
            MessageBoxA(g_main, "The custom port must be between 1 and 65535, or left blank.",
                        APP_TITLE, MB_OK | MB_ICONWARNING);
            return 0;
        }
        device->port = (int)port;
    }
    return 1;
}

static void clear_device_fields(void) {
    if (g_continuous_ping_active) {
        KillTimer(g_main, 2);
        g_continuous_ping_active = 0;
        set_text(GetDlgItem(g_main, ID_DEVICE_PING), "Ping");
    }
    SendMessage(h_device_list, LB_SETCURSEL, (WPARAM)-1, 0);
    set_text(h_device_name, "");
    set_text(h_device_ip, "");
    set_text(h_device_prefix, "24");
    SendMessage(h_device_protocol, CB_SETCURSEL, 0, 0);
    set_text(h_device_port, "");
    set_text(h_device_result, "Status: Ready");
    update_tools_context();
}

static void save_device(void) {
    DeviceEntry device;
    int i;
    int found = -1;
    if (!get_device_fields(&device, 1)) return;
    for (i = 0; i < g_config.device_count; ++i) {
        if (_stricmp(g_config.devices[i].name, device.name) == 0) {
            found = i;
            break;
        }
    }
    if (found >= 0) {
        g_config.devices[found] = device;
    } else {
        if (g_config.device_count >= MAX_DEVICES) {
            MessageBoxA(g_main, "The saved device limit has been reached.", APP_TITLE, MB_OK | MB_ICONWARNING);
            return;
        }
        found = g_config.device_count++;
        g_config.devices[found] = device;
    }
    if (save_config()) {
        populate_device_list();
        SendMessage(h_device_list, LB_SETCURSEL, found, 0);
        append_log("Saved device \"%s\" (%s).", device.name, device.address);
    }
}

static void delete_device(void) {
    int selection = (int)SendMessage(h_device_list, LB_GETCURSEL, 0, 0);
    int i;
    char name[128];
    if (selection < 0 || selection >= g_config.device_count) return;
    strncpy(name, g_config.devices[selection].name, sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';
    if (MessageBoxA(g_main, "Delete the selected saved device?", APP_TITLE,
                    MB_YESNO | MB_ICONQUESTION) != IDYES) return;
    for (i = selection; i + 1 < g_config.device_count; ++i) {
        g_config.devices[i] = g_config.devices[i + 1];
    }
    --g_config.device_count;
    if (save_config()) {
        populate_device_list();
        clear_device_fields();
        append_log("Deleted saved device \"%s\".", name);
    }
}

static void perform_ping(const char *address, int continuous) {
    HANDLE icmp;
    struct in_addr parsed;
    DWORD reply_size;
    void *reply_buffer;
    DWORD replies;
    DWORD error;
    char result[256];
    char payload[] = "NIC Equipment Manager";
    PICMP_ECHO_REPLY reply;

    if (InetPtonA(AF_INET, address, &parsed) != 1) return;
    icmp = IcmpCreateFile();
    if (icmp == INVALID_HANDLE_VALUE) {
        MessageBoxA(g_main, "Windows could not initialize ICMP ping.", APP_TITLE, MB_OK | MB_ICONERROR);
        return;
    }
    reply_size = sizeof(ICMP_ECHO_REPLY) + sizeof(payload) + 32;
    reply_buffer = calloc(1, reply_size);
    if (!reply_buffer) {
        IcmpCloseHandle(icmp);
        return;
    }
    set_text(h_device_result, "Status: Pinging...");
    SetCursor(LoadCursor(NULL, IDC_WAIT));
    replies = IcmpSendEcho(
        icmp,
        parsed.S_un.S_addr,
        payload,
        (WORD)sizeof(payload),
        NULL,
        reply_buffer,
        reply_size,
        continuous ? 800 : 1500
    );
    if (continuous) ++g_ping_sequence;
    if (replies > 0) {
        reply = (PICMP_ECHO_REPLY)reply_buffer;
        if (continuous) {
            ++g_ping_successes;
            snprintf(
                result,
                sizeof(result),
                "Continuous #%lu: Reachable - %lu ms  |  Success: %lu  Failed: %lu",
                g_ping_sequence,
                reply->RoundTripTime,
                g_ping_successes,
                g_ping_failures
            );
        } else {
            snprintf(result, sizeof(result), "Status: Reachable - %lu ms", reply->RoundTripTime);
        }
        set_text(h_device_result, result);
        if (!continuous) append_log("Ping %s succeeded in %lu ms.", address, reply->RoundTripTime);
    } else {
        error = GetLastError();
        if (continuous) {
            ++g_ping_failures;
            snprintf(
                result,
                sizeof(result),
                "Continuous #%lu: No response  |  Success: %lu  Failed: %lu",
                g_ping_sequence,
                g_ping_successes,
                g_ping_failures
            );
        } else if (error == IP_REQ_TIMED_OUT) {
            snprintf(result, sizeof(result), "Status: No ping response (timeout)");
        } else {
            snprintf(result, sizeof(result), "Status: Ping failed (Windows error %lu)", error);
        }
        set_text(h_device_result, result);
        if (!continuous) append_log("Ping %s failed (error %lu).", address, error);
    }
    SetCursor(LoadCursor(NULL, IDC_ARROW));
    free(reply_buffer);
    IcmpCloseHandle(icmp);
}

static void stop_continuous_ping(void) {
    if (!g_continuous_ping_active) return;
    KillTimer(g_main, 2);
    g_continuous_ping_active = 0;
    set_text(GetDlgItem(g_main, ID_DEVICE_PING), "Ping");
    append_log(
        "Stopped continuous ping to %s after %lu attempts (%lu successful, %lu failed).",
        g_continuous_ping_address,
        g_ping_sequence,
        g_ping_successes,
        g_ping_failures
    );
}

static void ping_device(void) {
    DeviceEntry device;
    int continuous;
    if (g_continuous_ping_active) {
        stop_continuous_ping();
        return;
    }
    if (!get_device_fields(&device, 0)) return;
    continuous = SendMessage(h_device_continuous, BM_GETCHECK, 0, 0) == BST_CHECKED;
    if (!continuous) {
        perform_ping(device.address, 0);
        return;
    }
    strncpy(g_continuous_ping_address, device.address, sizeof(g_continuous_ping_address) - 1);
    g_continuous_ping_address[sizeof(g_continuous_ping_address) - 1] = '\0';
    g_ping_sequence = 0;
    g_ping_successes = 0;
    g_ping_failures = 0;
    g_continuous_ping_active = 1;
    set_text(GetDlgItem(g_main, ID_DEVICE_PING), "Stop Ping");
    append_log("Started continuous ping to %s.", g_continuous_ping_address);
    perform_ping(g_continuous_ping_address, 1);
    SetTimer(g_main, 2, 1000, NULL);
}

static void open_device_webgui(void) {
    DeviceEntry device;
    char url[160];
    HINSTANCE result;
    if (!get_device_fields(&device, 0)) return;
    if (device.port > 0) {
        snprintf(url, sizeof(url), "%s://%s:%d", device.protocol, device.address, device.port);
    } else {
        snprintf(url, sizeof(url), "%s://%s", device.protocol, device.address);
    }
    result = ShellExecuteA(g_main, "open", url, NULL, NULL, SW_SHOWNORMAL);
    if ((INT_PTR)result <= 32) {
        MessageBoxA(g_main, "Windows could not open the default web browser.", APP_TITLE, MB_OK | MB_ICONERROR);
        append_log("Failed to open WebGUI: %s", url);
    } else {
        append_log("Opened WebGUI: %s", url);
    }
}

static void select_profile(void) {
    int selection = (int)SendMessage(h_profile_combo, CB_GETCURSEL, 0, 0);
    if (selection >= 0 && selection < g_config.profile_count) {
        load_profile_into_form(&g_config.profiles[selection]);
    }
}

static void select_device(void) {
    int selection = (int)SendMessage(h_device_list, LB_GETCURSEL, 0, 0);
    DeviceEntry *device;
    char port[16];
    char prefix[16];
    if (selection < 0 || selection >= g_config.device_count) return;
    if (g_continuous_ping_active) stop_continuous_ping();
    device = &g_config.devices[selection];
    set_text(h_device_name, device->name);
    set_text(h_device_ip, device->address);
    snprintf(prefix, sizeof(prefix), "%d", device->prefix ? device->prefix : 24);
    set_text(h_device_prefix, prefix);
    SendMessage(h_device_protocol, CB_SETCURSEL, _stricmp(device->protocol, "https") == 0 ? 1 : 0, 0);
    if (device->port > 0) {
        snprintf(port, sizeof(port), "%d", device->port);
        set_text(h_device_port, port);
    } else {
        set_text(h_device_port, "");
    }
    set_text(h_device_result, "Status: Ready");
    update_tools_context();
}

static void get_shell_context(ShellContext *context, char *device_ip, size_t device_ip_size) {
    AdapterInfo *adapter = selected_adapter();
    char prefix_text[32];
    int prefix = 0;
    memset(context, 0, sizeof(*context));
    get_text(h_device_ip, device_ip, (int)device_ip_size);
    trim(device_ip);
    get_text(h_device_prefix, prefix_text, sizeof(prefix_text));
    trim(prefix_text);
    if (!is_valid_ipv4(device_ip)) device_ip[0] = '\0';
    if (!parse_prefix(prefix_text, &prefix)) prefix = 0;
    if (adapter) {
        context->nic_index = adapter->if_index;
        context->nic_name = adapter->name;
    }
    context->device_ip = device_ip;
    context->device_prefix = prefix;
}

static void update_tools_context(void) {
    ShellContext context;
    char device_ip[32];
    char nic_text[420];
    char device_text[160];
    get_shell_context(&context, device_ip, sizeof(device_ip));
    if (context.nic_index) {
        snprintf(nic_text, sizeof(nic_text), "NIC context: %s  |  Interface index: %lu",
                 context.nic_name, context.nic_index);
    } else {
        strcpy(nic_text, "NIC context: No adapter selected");
    }
    if (context.device_ip && context.device_ip[0] && context.device_prefix > 0) {
        snprintf(device_text, sizeof(device_text), "Device context: %s/%d",
                 context.device_ip, context.device_prefix);
    } else {
        strcpy(device_text, "Device context: No valid device entered");
    }
    set_text(h_tools_context_nic, nic_text);
    set_text(h_tools_context_device, device_text);
    set_text(h_adv_context_nic, nic_text);
    set_text(h_adv_context_device, device_text);
}

static void launch_shell_tool(ShellTool tool) {
    ShellContext context;
    char device_ip[32];
    DWORD error = ERROR_SUCCESS;
    const char *name = tool == SHELL_TOOL_POWERSHELL ? "PowerShell" : "Command Prompt";
    char prompt[520];
    snprintf(
        prompt,
        sizeof(prompt),
        "Open an administrator %s window?\r\n\r\n"
        "Commands entered there can change Windows and network settings. The selected NIC and device are passed as environment variables.",
        name
    );
    if (MessageBoxA(g_main, prompt, APP_TITLE, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES) return;
    get_shell_context(&context, device_ip, sizeof(device_ip));
    if (!shell_tools_launch(g_main, tool, &context, &error)) {
        char message[256];
        snprintf(message, sizeof(message), "Windows could not open %s (error %lu).", name, error);
        MessageBoxA(g_main, message, APP_TITLE, MB_OK | MB_ICONERROR);
        append_log("Failed to open administrator %s (error %lu).", name, error);
        return;
    }
    append_log("Opened administrator %s with the selected NIC/device context.", name);
}

static void copy_tools_context(int copy_device) {
    ShellContext context;
    char device_ip[32];
    char text[320];
    get_shell_context(&context, device_ip, sizeof(device_ip));
    if (copy_device) {
        if (!context.device_ip || !context.device_ip[0] || context.device_prefix <= 0) {
            MessageBoxA(g_main, "Enter a valid device IPv4 address and subnet first.",
                        APP_TITLE, MB_OK | MB_ICONINFORMATION);
            return;
        }
        snprintf(text, sizeof(text), "%s/%d", context.device_ip, context.device_prefix);
    } else {
        if (!context.nic_index) {
            MessageBoxA(g_main, "Select a network adapter first.", APP_TITLE, MB_OK | MB_ICONINFORMATION);
            return;
        }
        snprintf(text, sizeof(text), "%s (Interface index %lu)", context.nic_name, context.nic_index);
    }
    if (shell_tools_copy_text(g_main, text)) append_log("Copied %s context to the clipboard.", copy_device ? "device" : "NIC");
    else append_log("Windows could not copy the selected context to the clipboard.");
}


static HWND make_advanced_control(
    DWORD ex_style,
    const char *class_name,
    const char *text,
    DWORD style,
    int x,
    int y,
    int width,
    int height,
    HWND parent,
    int id
) {
    HWND control = CreateWindowExA(
        ex_style,
        class_name,
        text,
        style | WS_CHILD | WS_VISIBLE,
        scale_px(x),
        scale_px(y),
        scale_px(width),
        scale_px(height),
        parent,
        (HMENU)(INT_PTR)id,
        g_instance,
        NULL
    );
    if (control) set_control_font(control);
    return control;
}

static HWND make_advanced_button(const char *text, int x, int y, int width, int height, HWND parent, int id) {
    return make_advanced_control(0, "BUTTON", text, BS_OWNERDRAW | WS_TABSTOP,
                                 x, y, width, height, parent, id);
}

static HWND make_advanced_groupbox(const char *title, int x, int y, int width, int height,
                                   HWND parent, int box_id, int header_id) {
    int header_width = measure_group_header_width(title, parent);
    HWND box = make_advanced_control(0, "BUTTON", "", BS_GROUPBOX,
                                     x, y, width, height, parent, box_id);
    HWND label = make_advanced_control(0, "STATIC", title, SS_CENTER | SS_CENTERIMAGE,
                                       x + 8, y - 1, header_width, 18, parent, header_id);
    if (label) SetPropA(label, "NIC_GROUP_HEADER", (HANDLE)1);
    return box;
}

static int run_command_capture(const char *command, char *output, size_t output_size, DWORD *exit_code) {
    SECURITY_ATTRIBUTES security;
    HANDLE read_pipe = NULL;
    HANDLE write_pipe = NULL;
    STARTUPINFOA startup;
    PROCESS_INFORMATION process;
    char command_line[32768];
    DWORD available = 0;
    DWORD bytes_read = 0;
    DWORD wait_result = WAIT_TIMEOUT;
    ULONGLONG started = GetTickCount64();
    size_t total = 0;
    int timed_out = 0;

    if (!command || !command[0] || !output || output_size == 0) return 0;
    output[0] = '\0';
    if (exit_code) *exit_code = ERROR_SUCCESS;

    memset(&security, 0, sizeof(security));
    security.nLength = sizeof(security);
    security.bInheritHandle = TRUE;
    if (!CreatePipe(&read_pipe, &write_pipe, &security, 0)) return 0;
    if (!SetHandleInformation(read_pipe, HANDLE_FLAG_INHERIT, 0)) {
        CloseHandle(read_pipe);
        CloseHandle(write_pipe);
        return 0;
    }

    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    startup.wShowWindow = SW_HIDE;
    startup.hStdOutput = write_pipe;
    startup.hStdError = write_pipe;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    memset(&process, 0, sizeof(process));

    snprintf(command_line, sizeof(command_line), "cmd.exe /D /S /C \"%s\"", command);
    command_line[sizeof(command_line) - 1] = '\0';

    if (!CreateProcessA(NULL, command_line, NULL, NULL, TRUE, CREATE_NO_WINDOW,
                        NULL, NULL, &startup, &process)) {
        CloseHandle(read_pipe);
        CloseHandle(write_pipe);
        return 0;
    }
    CloseHandle(write_pipe);
    write_pipe = NULL;

    for (;;) {
        while (total + 1 < output_size &&
               PeekNamedPipe(read_pipe, NULL, 0, NULL, &available, NULL) && available > 0) {
            DWORD request = (DWORD)(output_size - total - 1);
            if (request > available) request = available;
            if (!ReadFile(read_pipe, output + total, request, &bytes_read, NULL) || bytes_read == 0) break;
            total += bytes_read;
            output[total] = '\0';
        }

        wait_result = WaitForSingleObject(process.hProcess, 50);
        if (wait_result == WAIT_OBJECT_0) {
            if (!PeekNamedPipe(read_pipe, NULL, 0, NULL, &available, NULL) || available == 0) break;
        } else if (wait_result == WAIT_FAILED) {
            break;
        }

        if (GetTickCount64() - started > 120000ULL) {
            TerminateProcess(process.hProcess, ERROR_TIMEOUT);
            timed_out = 1;
            wait_result = WAIT_TIMEOUT;
            break;
        }
    }

    while (total + 1 < output_size &&
           ReadFile(read_pipe, output + total, (DWORD)(output_size - total - 1), &bytes_read, NULL) &&
           bytes_read > 0) {
        total += bytes_read;
    }
    output[total] = '\0';

    if (timed_out) {
        const char *message = "\r\nCommand timed out after 120 seconds.\r\n";
        size_t remaining = output_size - total;
        if (remaining > 1) strncat(output, message, remaining - 1);
    }
    if (exit_code) GetExitCodeProcess(process.hProcess, exit_code);

    CloseHandle(read_pipe);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return wait_result == WAIT_OBJECT_0;
}


static LRESULT CALLBACK advanced_output_subclass_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR subclass_id, DWORD_PTR ref_data) {
    LRESULT result;
    int repaint = 0;
    (void)ref_data;

    switch (message) {
        case WM_VSCROLL:
        case WM_HSCROLL:
        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
            repaint = 1;
            break;
        case WM_KEYDOWN:
            if (wparam == VK_UP || wparam == VK_DOWN || wparam == VK_PRIOR || wparam == VK_NEXT ||
                wparam == VK_HOME || wparam == VK_END || wparam == VK_LEFT || wparam == VK_RIGHT) {
                repaint = 1;
            }
            break;
        case WM_NCDESTROY:
            RemoveWindowSubclass(hwnd, advanced_output_subclass_proc, subclass_id);
            break;
    }

    result = DefSubclassProc(hwnd, message, wparam, lparam);
    if (repaint && IsWindow(hwnd)) {
        RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
    }
    return result;
}

static void advanced_set_output(const char *text) {
    set_text(h_adv_output, text && text[0] ? text : "(Command completed with no output.)\r\n");
    if (h_adv_output) SendMessage(h_adv_output, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
}

static void run_advanced_command_text(const char *command) {
    char output[65536];
    DWORD exit_code = ERROR_SUCCESS;
    char status[256];
    if (!command || !command[0]) return;
    set_text(h_adv_command, command);
    SetCursor(LoadCursor(NULL, IDC_WAIT));
    if (!run_command_capture(command, output, sizeof(output), &exit_code)) {
        if (!output[0]) snprintf(output, sizeof(output), "Windows could not run the command (error %lu).\r\n", GetLastError());
    }
    SetCursor(LoadCursor(NULL, IDC_ARROW));
    advanced_set_output(output);
    snprintf(status, sizeof(status), "Advanced Tools command completed with exit code %lu: %s", exit_code, command);
    append_log("%s", status);
    if (h_tools_status) {
        char panel_status[256];
        snprintf(panel_status, sizeof(panel_status), "Status: Last command exit code %lu - %.180s", exit_code, command);
        set_text(h_tools_status, panel_status);
    }
}

static void run_advanced_preset(DiagnosticCommand preset) {
    ShellContext context;
    char device_ip[32];
    char command[512];
    get_shell_context(&context, device_ip, sizeof(device_ip));
    if (!diagnostics_build_command(preset, context.device_ip, command, sizeof(command))) {
        MessageBoxA(g_advanced_window,
                    "This diagnostic requires a valid device IPv4 address in the main window.",
                    "Advanced Tools", MB_OK | MB_ICONINFORMATION);
        return;
    }
    run_advanced_command_text(command);
}

static void save_advanced_output(void) {
    char output[65536];
    char path[MAX_PATH];
    FILE *file;
    get_text(h_adv_output, output, sizeof(output));
    if (!output[0]) {
        MessageBoxA(g_advanced_window, "There is no output to save.", "Advanced Tools", MB_OK | MB_ICONINFORMATION);
        return;
    }
    build_app_path(path, sizeof(path), "NICEquipmentManager-AdvancedTools-output.txt");
    file = fopen(path, "wb");
    if (!file) {
        MessageBoxA(g_advanced_window, "The output file could not be written to the application folder.",
                    "Advanced Tools", MB_OK | MB_ICONERROR);
        return;
    }
    fwrite(output, 1, strlen(output), file);
    fclose(file);
    append_log("Saved Advanced Tools output to %s.", path);
    MessageBoxA(g_advanced_window, "Output saved beside the application executable.",
                "Advanced Tools", MB_OK | MB_ICONINFORMATION);
}

static void layout_advanced_window(HWND hwnd, int width, int height) {
    int margin = scale_px(10);
    int inner = width - margin * 2;
    int output_top = scale_px(333);
    int bottom = height - scale_px(48);
    int output_height = bottom - output_top;
    if (output_height < scale_px(100)) output_height = scale_px(100);

    MoveWindow(GetDlgItem(hwnd, ID_ADV_GROUP_CONTEXT), margin, scale_px(8), inner, scale_px(70), TRUE);
    MoveWindow(h_adv_context_nic, scale_px(25), scale_px(27), width - scale_px(50), scale_px(20), TRUE);
    MoveWindow(h_adv_context_device, scale_px(25), scale_px(49), width - scale_px(50), scale_px(20), TRUE);
    MoveWindow(GetDlgItem(hwnd, ID_ADV_GROUP_SHELLS), margin, scale_px(84), inner, scale_px(60), TRUE);
    MoveWindow(GetDlgItem(hwnd, ID_ADV_GROUP_DIAG), margin, scale_px(150), inner, scale_px(100), TRUE);
    MoveWindow(GetDlgItem(hwnd, ID_ADV_GROUP_WORKSPACE), margin, scale_px(256), inner, height - scale_px(266), TRUE);
    MoveWindow(h_adv_command, scale_px(25), scale_px(282), width - scale_px(130), scale_px(27), TRUE);
    MoveWindow(GetDlgItem(hwnd, ID_ADV_RUN), width - scale_px(95), scale_px(282), scale_px(70), scale_px(27), TRUE);
    MoveWindow(h_adv_output, scale_px(25), output_top, width - scale_px(50), output_height, TRUE);
    MoveWindow(GetDlgItem(hwnd, ID_ADV_COPY), scale_px(25), bottom + scale_px(8), scale_px(90), scale_px(28), TRUE);
    MoveWindow(GetDlgItem(hwnd, ID_ADV_CLEAR), scale_px(125), bottom + scale_px(8), scale_px(90), scale_px(28), TRUE);
    MoveWindow(GetDlgItem(hwnd, ID_ADV_SAVE), scale_px(225), bottom + scale_px(8), scale_px(110), scale_px(28), TRUE);
}

static void create_advanced_ui(HWND hwnd) {
    make_advanced_groupbox("Current Context", 10, 8, 840, 70, hwnd, ID_ADV_GROUP_CONTEXT, ID_ADV_HEADER_CONTEXT);
    h_adv_context_nic = make_advanced_control(0, "STATIC", "NIC context: No adapter selected",
                                              SS_LEFT | SS_CENTERIMAGE, 25, 27, 810, 20, hwnd, 0);
    h_adv_context_device = make_advanced_control(0, "STATIC", "Device context: No valid device entered",
                                                 SS_LEFT | SS_CENTERIMAGE, 25, 49, 810, 20, hwnd, 0);

    make_advanced_groupbox("Administrator Shells", 10, 84, 840, 60, hwnd, ID_ADV_GROUP_SHELLS, ID_ADV_HEADER_SHELLS);
    make_advanced_button("PowerShell", 25, 105, 130, 28, hwnd, ID_ADV_POWERSHELL);
    make_advanced_button("Command Prompt", 165, 105, 150, 28, hwnd, ID_ADV_CMD);

    make_advanced_groupbox("Network Diagnostics", 10, 150, 840, 100, hwnd, ID_ADV_GROUP_DIAG, ID_ADV_HEADER_DIAG);
    make_advanced_button("ipconfig /all", 25, 172, 120, 28, hwnd, ID_ADV_IPCONFIG);
    make_advanced_button("route print", 155, 172, 110, 28, hwnd, ID_ADV_ROUTE);
    make_advanced_button("arp -a", 275, 172, 90, 28, hwnd, ID_ADV_ARP);
    make_advanced_button("netstat", 375, 172, 90, 28, hwnd, ID_ADV_NETSTAT);
    make_advanced_button("tracert target", 25, 207, 120, 28, hwnd, ID_ADV_TRACERT);
    make_advanced_button("pathping target", 155, 207, 130, 28, hwnd, ID_ADV_PATHPING);
    make_advanced_button("nslookup target", 295, 207, 130, 28, hwnd, ID_ADV_NSLOOKUP);

    make_advanced_groupbox("Command Workspace - Runs with Administrator Rights", 10, 256, 840, 330, hwnd, ID_ADV_GROUP_WORKSPACE, ID_ADV_HEADER_WORKSPACE);
    h_adv_command = make_advanced_control(WS_EX_CLIENTEDGE, "EDIT", "", ES_AUTOHSCROLL | WS_TABSTOP,
                                          25, 282, 725, 27, hwnd, ID_ADV_COMMAND);
    make_advanced_button("Run", 760, 282, 70, 27, hwnd, ID_ADV_RUN);
    h_adv_output = make_advanced_control(WS_EX_CLIENTEDGE, "EDIT", "Ready. Select a diagnostic preset or enter a command.\r\n",
                                         ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_READONLY |
                                         WS_VSCROLL | WS_HSCROLL | WS_TABSTOP,
                                         25, 333, 805, 205, hwnd, ID_ADV_OUTPUT);
    SendMessage(h_adv_output, EM_SETLIMITTEXT, 1024 * 1024, 0);
    SetWindowSubclass(h_adv_output, advanced_output_subclass_proc, 1, 0);
    make_advanced_button("Copy", 25, 546, 90, 28, hwnd, ID_ADV_COPY);
    make_advanced_button("Clear", 125, 546, 90, 28, hwnd, ID_ADV_CLEAR);
    make_advanced_button("Save Output", 225, 546, 110, 28, hwnd, ID_ADV_SAVE);
    update_tools_context();
}

static void apply_theme_to_window(HWND hwnd) {
    BOOL dark_title = g_dark_theme ? TRUE : FALSE;
    HRESULT result = DwmSetWindowAttribute(hwnd, 20, &dark_title, sizeof(dark_title));
    if (FAILED(result)) DwmSetWindowAttribute(hwnd, 19, &dark_title, sizeof(dark_title));
    SetClassLongPtrA(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)g_background_brush);
    SetWindowTheme(hwnd, L"", L"");
    EnumChildWindows(hwnd, apply_theme_to_child, 0);
    RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_FRAME);
}

static LRESULT CALLBACK advanced_window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    int id;
    switch (message) {
        case WM_CREATE:
            g_advanced_window = hwnd;
            create_advanced_ui(hwnd);
            apply_theme_to_window(hwnd);
            return 0;
        case WM_SIZE:
            if (wparam != SIZE_MINIMIZED) layout_advanced_window(hwnd, LOWORD(lparam), HIWORD(lparam));
            return 0;
        case WM_GETMINMAXINFO:
        {
            MINMAXINFO *limits = (MINMAXINFO *)lparam;
            limits->ptMinTrackSize.x = scale_px(700);
            limits->ptMinTrackSize.y = scale_px(560);
            return 0;
        }
        case WM_ERASEBKGND:
        {
            RECT client;
            GetClientRect(hwnd, &client);
            FillRect((HDC)wparam, &client, g_background_brush);
            return 1;
        }
        case WM_CTLCOLORSTATIC:
        {
            HDC dc = (HDC)wparam;
            SetTextColor(dc, g_text_color);
            if (GetPropA((HWND)lparam, "NIC_GROUP_HEADER")) {
                SetBkMode(dc, OPAQUE);
                SetBkColor(dc, g_background_color);
            } else {
                SetBkMode(dc, TRANSPARENT);
            }
            return (LRESULT)g_background_brush;
        }
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
        {
            HDC dc = (HDC)wparam;
            SetTextColor(dc, g_text_color);
            SetBkColor(dc, g_edit_color);
            return (LRESULT)g_edit_brush;
        }
        case WM_CTLCOLORBTN:
        {
            HDC dc = (HDC)wparam;
            SetTextColor(dc, g_text_color);
            SetBkColor(dc, g_background_color);
            return (LRESULT)g_background_brush;
        }
        case WM_DRAWITEM:
        {
            DRAWITEMSTRUCT *item = (DRAWITEMSTRUCT *)lparam;
            if (item->CtlType == ODT_BUTTON) {
                draw_owner_button(item);
                return TRUE;
            }
            break;
        }
        case WM_COMMAND:
            id = LOWORD(wparam);
            if (id == ID_ADV_POWERSHELL) launch_shell_tool(SHELL_TOOL_POWERSHELL);
            else if (id == ID_ADV_CMD) launch_shell_tool(SHELL_TOOL_COMMAND_PROMPT);
            else if (id == ID_ADV_IPCONFIG) run_advanced_preset(DIAG_IPCONFIG_ALL);
            else if (id == ID_ADV_ROUTE) run_advanced_preset(DIAG_ROUTE_PRINT);
            else if (id == ID_ADV_ARP) run_advanced_preset(DIAG_ARP);
            else if (id == ID_ADV_NETSTAT) run_advanced_preset(DIAG_NETSTAT);
            else if (id == ID_ADV_TRACERT) run_advanced_preset(DIAG_TRACERT);
            else if (id == ID_ADV_PATHPING) run_advanced_preset(DIAG_PATHPING);
            else if (id == ID_ADV_NSLOOKUP) run_advanced_preset(DIAG_NSLOOKUP);
            else if (id == ID_ADV_RUN) {
                char command[4096];
                get_text(h_adv_command, command, sizeof(command));
                trim(command);
                if (command[0]) run_advanced_command_text(command);
            } else if (id == ID_ADV_COPY) {
                char output[65536];
                get_text(h_adv_output, output, sizeof(output));
                if (shell_tools_copy_text(hwnd, output)) append_log("Copied Advanced Tools output to the clipboard.");
            } else if (id == ID_ADV_CLEAR) {
                set_text(h_adv_output, "");
            } else if (id == ID_ADV_SAVE) {
                save_advanced_output();
            }
            return 0;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            g_advanced_window = NULL;
            h_adv_context_nic = NULL;
            h_adv_context_device = NULL;
            h_adv_command = NULL;
            h_adv_output = NULL;
            return 0;
    }
    return DefWindowProcA(hwnd, message, wparam, lparam);
}

static void open_advanced_tools(void) {
    if (g_advanced_window && IsWindow(g_advanced_window)) {
        ShowWindow(g_advanced_window, SW_RESTORE);
        SetForegroundWindow(g_advanced_window);
        update_tools_context();
        return;
    }
    g_advanced_window = CreateWindowExA(
        WS_EX_TOOLWINDOW,
        "NICEquipmentManagerAdvancedWindow",
        APP_TITLE " - Advanced Tools",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        scale_px(880),
        scale_px(640),
        g_main,
        NULL,
        g_instance,
        NULL
    );
    if (!g_advanced_window) {
        MessageBoxA(g_main, "The Advanced Tools window could not be opened.", APP_TITLE, MB_OK | MB_ICONERROR);
        return;
    }
    append_log("Opened the Advanced Tools workspace.");
    if (h_tools_status) set_text(h_tools_status, "Status: Advanced Tools workspace open");
}

static HMENU create_main_menu(void) {
    HMENU menu = CreateMenu();
    HMENU tools = CreatePopupMenu();
    AppendMenuA(tools, MF_STRING, ID_TOOLS_OPEN_ADVANCED, "Open &Advanced Tools...");
    AppendMenuA(tools, MF_SEPARATOR, 0, NULL);
    AppendMenuA(tools, MF_STRING, ID_TOOLS_POWERSHELL, "Open Administrator &PowerShell");
    AppendMenuA(tools, MF_STRING, ID_TOOLS_CMD, "Open Administrator &Command Prompt");
    AppendMenuA(tools, MF_SEPARATOR, 0, NULL);
    AppendMenuA(tools, MF_STRING, ID_TOOLS_COPY_NIC, "Copy Selected &NIC Context");
    AppendMenuA(tools, MF_STRING, ID_TOOLS_COPY_DEVICE, "Copy Selected &Device Context");
    AppendMenuA(menu, MF_POPUP, (UINT_PTR)tools, "&Tools");
    return menu;
}

static void create_ui(HWND hwnd) {
    make_groupbox("Network Adapter Status", 10, 8, 1145, 88, hwnd, ID_GROUP_HEADER_ADAPTER);
    make_label("Adapter:", 25, 28, 55, 24, hwnd, 0);
    h_adapter_combo = make_control(
        WS_EX_CLIENTEDGE, "COMBOBOX", "",
        CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_TABSTOP | WS_VSCROLL,
        88, 28, 377, 300, hwnd, ID_ADAPTER_COMBO
    );
    make_button("Refresh", 475, 27, 80, 26, hwnd, ID_REFRESH);
    h_adapter_status = make_label("Status: --", 575, 25, 210, 25, hwnd, ID_ADAPTER_STATUS);
    h_adapter_link = make_label("Link speed: --", 790, 25, 220, 25, hwnd, ID_ADAPTER_LINK);
    make_label("Theme:", 1015, 25, 48, 25, hwnd, 0);
    h_theme_combo = make_control(
        WS_EX_CLIENTEDGE, "COMBOBOX", "",
        CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_TABSTOP,
        1063, 25, 78, 110, hwnd, ID_THEME_COMBO
    );
    SendMessageA(h_theme_combo, CB_ADDSTRING, 0, (LPARAM)"System");
    SendMessageA(h_theme_combo, CB_ADDSTRING, 0, (LPARAM)"Light");
    SendMessageA(h_theme_combo, CB_ADDSTRING, 0, (LPARAM)"Dark");
    SendMessage(h_theme_combo, CB_SETCURSEL, 0, 0);
    h_adapter_desc = make_label("Description: --", 25, 55, 520, 25, hwnd, ID_ADAPTER_DESC);
    h_adapter_addrs = make_label("IPv4: --", 575, 55, 560, 25, hwnd, ID_ADAPTER_ADDRS);

    make_groupbox("IPv4 Configuration and Profiles", 10, 102, 565, 425, hwnd, ID_GROUP_HEADER_PROFILE);
    make_label("Saved profile:", 25, 124, 90, 24, hwnd, 0);
    h_profile_combo = make_control(
        WS_EX_CLIENTEDGE, "COMBOBOX", "",
        CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_TABSTOP | WS_VSCROLL,
        123, 123, 202, 250, hwnd, ID_PROFILE_COMBO
    );
    make_button("Load", 332, 122, 62, 26, hwnd, ID_PROFILE_APPLY);
    make_button("Delete", 400, 122, 65, 26, hwnd, ID_PROFILE_DELETE);

    make_label("Profile name:", 25, 157, 90, 24, hwnd, 0);
    h_profile_name = make_edit("", 123, 157, 202, 24, hwnd, ID_PROFILE_NAME);
    make_button("Save / Update", 332, 156, 133, 26, hwnd, ID_PROFILE_SAVE);

    h_dhcp = make_control(0, "BUTTON", "Use DHCP (automatic IPv4 and DNS)",
                          BS_AUTOCHECKBOX | WS_TABSTOP, 25, 191, 300, 24, hwnd, ID_DHCP);

    make_label("Primary IPv4:", 25, 224, 90, 24, hwnd, 0);
    h_primary_ip = make_edit("", 123, 224, 137, 24, hwnd, ID_PRIMARY_IP);
    make_label("Prefix/mask:", 275, 224, 80, 24, hwnd, 0);
    h_prefix = make_edit("24", 363, 224, 102, 24, hwnd, ID_PREFIX);

    make_label("Gateway:", 25, 255, 90, 24, hwnd, 0);
    h_gateway = make_edit("", 123, 255, 137, 24, hwnd, ID_GATEWAY);
    make_label("DNS 1:", 275, 255, 80, 24, hwnd, 0);
    h_dns1 = make_edit("", 363, 255, 102, 24, hwnd, ID_DNS1);

    make_label("DNS 2:", 275, 286, 80, 24, hwnd, 0);
    h_dns2 = make_edit("", 363, 286, 102, 24, hwnd, ID_DNS2);

    make_label("Additional IPv4 networks (no gateway):", 25, 318, 260, 24, hwnd, 0);
    h_secondary_list = make_control(
        WS_EX_CLIENTEDGE, "LISTBOX", "", LBS_NOTIFY | WS_TABSTOP | WS_VSCROLL,
        25, 345, 300, 112, hwnd, ID_SECONDARY_LIST
    );
    make_label("Address:", 337, 344, 60, 24, hwnd, 0);
    h_secondary_ip = make_edit("", 405, 344, 142, 24, hwnd, ID_SECONDARY_IP);
    make_label("Prefix/mask:", 337, 376, 80, 24, hwnd, 0);
    h_secondary_prefix = make_edit("24", 425, 376, 122, 24, hwnd, ID_SECONDARY_PREFIX);
    make_button("Add", 337, 408, 100, 27, hwnd, ID_SECONDARY_ADD);
    make_button("Remove", 447, 408, 100, 27, hwnd, ID_SECONDARY_REMOVE);

    make_button("Load Current", 25, 473, 120, 32, hwnd, ID_LOAD_CURRENT);
    make_button("Apply to Selected NIC", 337, 473, 210, 32, hwnd, ID_APPLY_CURRENT);

    make_groupbox("Device Access - Saved List and Free-form IPv4", 585, 102, 570, 425, hwnd, ID_GROUP_HEADER_DEVICE);
    make_label("Saved devices:", 600, 124, 110, 24, hwnd, 0);
    h_device_list = make_control(
        WS_EX_CLIENTEDGE, "LISTBOX", "", LBS_NOTIFY | WS_TABSTOP | WS_VSCROLL,
        600, 150, 535, 165, hwnd, ID_DEVICE_LIST
    );

    make_label("Name:", 600, 327, 55, 24, hwnd, 0);
    h_device_name = make_edit("", 663, 327, 182, 24, hwnd, ID_DEVICE_NAME);
    make_label("IPv4:", 855, 327, 45, 24, hwnd, 0);
    h_device_ip = make_edit("", 908, 327, 227, 24, hwnd, ID_DEVICE_IP);

    make_label("Subnet/prefix:", 600, 359, 90, 24, hwnd, 0);
    h_device_prefix = make_edit("24", 698, 359, 97, 24, hwnd, ID_DEVICE_PREFIX);
    make_label("Protocol:", 810, 359, 60, 24, hwnd, 0);
    h_device_protocol = make_control(
        WS_EX_CLIENTEDGE, "COMBOBOX", "",
        CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_TABSTOP,
        878, 359, 82, 120, hwnd, ID_DEVICE_PROTOCOL
    );
    SendMessageA(h_device_protocol, CB_ADDSTRING, 0, (LPARAM)"HTTP");
    SendMessageA(h_device_protocol, CB_ADDSTRING, 0, (LPARAM)"HTTPS");
    SendMessage(h_device_protocol, CB_SETCURSEL, 0, 0);
    make_label("Port:", 975, 359, 40, 24, hwnd, 0);
    h_device_port = make_edit("", 1023, 359, 112, 24, hwnd, ID_DEVICE_PORT);

    make_button("Save / Update", 600, 398, 125, 30, hwnd, ID_DEVICE_SAVE);
    make_button("Delete", 733, 398, 80, 30, hwnd, ID_DEVICE_DELETE);
    make_button("Clear / New", 821, 398, 105, 30, hwnd, ID_DEVICE_CLEAR);
    make_button("Ping", 934, 398, 80, 30, hwnd, ID_DEVICE_PING);
    make_button("Open WebGUI", 1022, 398, 113, 30, hwnd, ID_DEVICE_OPEN);
    h_device_continuous = make_control(
        0, "BUTTON", "Continuous", BS_AUTOCHECKBOX | WS_TABSTOP,
        930, 433, 100, 24, hwnd, ID_DEVICE_CONTINUOUS
    );
    h_device_result = make_label("Status: Ready", 600, 459, 535, 27, hwnd, ID_DEVICE_RESULT);
    make_label(
        "Tip: WebGUI can open even when a device blocks ping. HTTPS equipment may show a certificate warning.",
        600, 488, 535, 28, hwnd, 0
    );

    g_register_advanced = 1;
    make_groupbox("Advanced Tools", 10, 534, 1145, 94, hwnd, ID_GROUP_HEADER_ADVANCED);
    h_tools_context_nic = make_label("NIC context: No adapter selected",
                                     25, 556, 735, 20, hwnd, ID_TOOLS_CONTEXT_NIC);
    h_tools_context_device = make_label("Device context: No valid device entered",
                                        25, 578, 735, 20, hwnd, ID_TOOLS_CONTEXT_DEVICE);
    h_tools_status = make_label("Status: Ready",
                                25, 600, 735, 20, hwnd, ID_TOOLS_STATUS);
    make_button("Open Advanced Tools", 950, 572, 190, 30, hwnd, ID_TOOLS_OPEN_ADVANCED);
    g_register_advanced = 0;

    make_groupbox("Activity Log", 10, 534, 1145, 135, hwnd, ID_GROUP_HEADER_LOG);
    h_log_list = make_control(
        WS_EX_CLIENTEDGE, "LISTBOX", "", WS_VSCROLL | LBS_NOINTEGRALHEIGHT,
        25, 556, 1110, 92, hwnd, ID_LOG_LIST
    );
    h_status_line = make_label("Ready", 15, 674, 1125, 25, hwnd, ID_STATUS_LINE);
}

static void save_window_state(void) {
    WINDOWPLACEMENT placement;
    RECT normal;
    memset(&placement, 0, sizeof(placement));
    placement.length = sizeof(placement);
    if (!g_main || !GetWindowPlacement(g_main, &placement)) return;
    normal = placement.rcNormalPosition;
    g_config.window_saved = 1;
    g_config.window_x = normal.left;
    g_config.window_y = normal.top;
    g_config.window_width = normal.right - normal.left;
    g_config.window_height = normal.bottom - normal.top;
    g_config.window_maximized = placement.showCmd == SW_SHOWMAXIMIZED;
}

static int saved_window_is_visible(void) {
    RECT rect;
    if (!g_config.window_saved ||
        g_config.window_width < scale_px(MAIN_MIN_CLIENT_WIDTH) ||
        g_config.window_height < scale_px(MAIN_MIN_CLIENT_HEIGHT)) return 0;
    rect.left = g_config.window_x;
    rect.top = g_config.window_y;
    rect.right = rect.left + g_config.window_width;
    rect.bottom = rect.top + g_config.window_height;
    return MonitorFromRect(&rect, MONITOR_DEFAULTTONULL) != NULL;
}

static int system_dpi(void) {
    HDC screen = GetDC(NULL);
    int dpi = screen ? GetDeviceCaps(screen, LOGPIXELSX) : 96;
    if (screen) ReleaseDC(NULL, screen);
    return dpi > 0 ? dpi : 96;
}

static int ensure_administrator(void) {
    char executable[MAX_PATH];
    HINSTANCE result;
    if (IsUserAnAdmin()) return 1;
    GetModuleFileNameA(NULL, executable, sizeof(executable));
    result = ShellExecuteA(NULL, "runas", executable, NULL, NULL, SW_SHOWNORMAL);
    if ((INT_PTR)result <= 32) {
        MessageBoxA(NULL,
                    "Administrator permission is required to change network adapter settings.",
                    APP_TITLE,
                    MB_OK | MB_ICONWARNING);
    }
    return 0;
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    int id;
    int notification;
    switch (message) {
        case WM_CREATE:
            g_main = hwnd;
            create_ui(hwnd);
            ui_layout_set_base(&g_layout, hwnd);
            {
                RECT client;
                GetClientRect(hwnd, &client);
                ui_layout_resize(
                    &g_layout,
                    client.right,
                    client.bottom,
                    scale_px(ADVANCED_PANEL_MIN_CLIENT_HEIGHT)
                );
            }
            if (!g_config_load_ok) {
                append_log("The configuration file was invalid, so an empty configuration was loaded.");
            }
            apply_theme();
            populate_profile_list();
            populate_device_list();
            refresh_adapters(1);
            append_log("%s %s started.", APP_TITLE, APP_VERSION);
            SetTimer(hwnd, 1, 5000, NULL);
            return 0;

        case WM_SIZE:
            if (wparam != SIZE_MINIMIZED) {
                ui_layout_resize(
                    &g_layout,
                    LOWORD(lparam),
                    HIWORD(lparam),
                    scale_px(ADVANCED_PANEL_MIN_CLIENT_HEIGHT)
                );
                update_tools_context();
            }
            return 0;

        case WM_GETMINMAXINFO:
        {
            MINMAXINFO *limits = (MINMAXINFO *)lparam;
            get_main_min_track_size(hwnd, &limits->ptMinTrackSize);
            return 0;
        }

        case WM_TIMER:
            if (wparam == 1) refresh_adapters(0);
            else if (wparam == 2 && g_continuous_ping_active) {
                perform_ping(g_continuous_ping_address, 1);
            }
            return 0;

        case WM_SETTINGCHANGE:
            if (_stricmp(g_config.theme, "system") == 0) apply_theme();
            return 0;

        case WM_ERASEBKGND:
        {
            RECT client;
            GetClientRect(hwnd, &client);
            FillRect((HDC)wparam, &client, g_background_brush);
            return 1;
        }

        case WM_CTLCOLORSTATIC:
        {
            HDC dc = (HDC)wparam;
            SetTextColor(dc, g_text_color);
            if (GetPropA((HWND)lparam, "NIC_GROUP_HEADER")) {
                SetBkMode(dc, OPAQUE);
                SetBkColor(dc, g_background_color);
            } else {
                SetBkMode(dc, TRANSPARENT);
            }
            return (LRESULT)g_background_brush;
        }

        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
        {
            HDC dc = (HDC)wparam;
            SetTextColor(dc, g_text_color);
            SetBkColor(dc, g_edit_color);
            return (LRESULT)g_edit_brush;
        }

        case WM_CTLCOLORBTN:
        {
            HDC dc = (HDC)wparam;
            SetTextColor(dc, g_text_color);
            SetBkColor(dc, g_background_color);
            return (LRESULT)g_background_brush;
        }

        case WM_CTLCOLORSCROLLBAR:
            return (LRESULT)g_background_brush;

        case WM_MEASUREITEM:
        {
            MEASUREITEMSTRUCT *measure = (MEASUREITEMSTRUCT *)lparam;
            if (measure->CtlType == ODT_COMBOBOX) {
                measure->itemHeight = (UINT)scale_px(22);
                return TRUE;
            }
            break;
        }

        case WM_DRAWITEM:
        {
            DRAWITEMSTRUCT *item = (DRAWITEMSTRUCT *)lparam;
            if (item->CtlType == ODT_BUTTON) {
                draw_owner_button(item);
                return TRUE;
            }
            if (item->CtlType == ODT_COMBOBOX) {
                draw_owner_combo(item);
                return TRUE;
            }
            break;
        }

        case WM_COMMAND:
            id = LOWORD(wparam);
            notification = HIWORD(wparam);
            if (id == ID_ADAPTER_COMBO && notification == CBN_SELCHANGE) {
                update_adapter_summary();
                load_adapter_into_form();
            } else if (id == ID_REFRESH) {
                refresh_adapters(1);
                append_log("Refreshed network adapter status.");
            } else if (id == ID_THEME_COMBO && notification == CBN_SELCHANGE) {
                int theme_selection = (int)SendMessage(h_theme_combo, CB_GETCURSEL, 0, 0);
                if (theme_selection == 1) strcpy(g_config.theme, "light");
                else if (theme_selection == 2) strcpy(g_config.theme, "dark");
                else strcpy(g_config.theme, "system");
                apply_theme();
                save_config();
                append_log("Theme set to %s.", theme_selection == 0 ? "follow Windows" :
                           (theme_selection == 1 ? "Light" : "Dark"));
            } else if (id == ID_DHCP && notification == BN_CLICKED) {
                toggle_static_controls();
            } else if (id == ID_PROFILE_COMBO && notification == CBN_SELCHANGE) {
                select_profile();
            } else if (id == ID_PROFILE_APPLY) {
                select_profile();
            } else if (id == ID_PROFILE_SAVE) {
                save_profile();
            } else if (id == ID_PROFILE_DELETE) {
                delete_profile();
            } else if (id == ID_SECONDARY_ADD) {
                add_secondary_address();
            } else if (id == ID_SECONDARY_REMOVE) {
                remove_secondary_address();
            } else if (id == ID_LOAD_CURRENT) {
                load_adapter_into_form();
                append_log("Loaded current settings from the selected adapter.");
            } else if (id == ID_APPLY_CURRENT) {
                apply_network_settings();
            } else if (id == ID_DEVICE_LIST && notification == LBN_SELCHANGE) {
                select_device();
            } else if (id == ID_DEVICE_SAVE) {
                save_device();
            } else if (id == ID_DEVICE_DELETE) {
                delete_device();
            } else if (id == ID_DEVICE_CLEAR) {
                clear_device_fields();
            } else if (id == ID_DEVICE_PING) {
                ping_device();
            } else if (id == ID_DEVICE_CONTINUOUS && notification == BN_CLICKED) {
                if (g_continuous_ping_active &&
                    SendMessage(h_device_continuous, BM_GETCHECK, 0, 0) != BST_CHECKED) {
                    stop_continuous_ping();
                }
            } else if (id == ID_DEVICE_OPEN) {
                open_device_webgui();
            } else if (id == ID_DEVICE_IP || id == ID_DEVICE_PREFIX) {
                if (notification == EN_CHANGE) update_tools_context();
            } else if (id == ID_TOOLS_OPEN_ADVANCED) {
                open_advanced_tools();
            } else if (id == ID_TOOLS_POWERSHELL) {
                launch_shell_tool(SHELL_TOOL_POWERSHELL);
            } else if (id == ID_TOOLS_CMD) {
                launch_shell_tool(SHELL_TOOL_COMMAND_PROMPT);
            } else if (id == ID_TOOLS_COPY_NIC) {
                copy_tools_context(0);
            } else if (id == ID_TOOLS_COPY_DEVICE) {
                copy_tools_context(1);
            }
            return 0;

        case WM_CLOSE:
            save_window_state();
            save_config();
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            KillTimer(hwnd, 1);
            KillTimer(hwnd, 2);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcA(hwnd, message, wparam, lparam);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR command_line, int show_command) {
    WNDCLASSEXA window_class;
    MSG message;
    WSADATA winsock;
    INITCOMMONCONTROLSEX common_controls;
    NONCLIENTMETRICSA metrics;
    HMENU main_menu;
    int initial_x = CW_USEDEFAULT;
    int initial_y = CW_USEDEFAULT;
    int initial_width;
    int initial_height;

    (void)previous;
    (void)command_line;
    if (!ensure_administrator()) return 1;

    SetProcessDPIAware();
    g_dpi = system_dpi();
    ui_layout_init(&g_layout, g_dpi);

    g_instance = instance;
    build_app_path(g_config_path, sizeof(g_config_path), "NICEquipmentManager.json");
    build_app_path(g_log_path, sizeof(g_log_path), "NICEquipmentManager.log");
    build_app_path(g_backup_path, sizeof(g_backup_path), "NICEquipmentManager-last-backup.txt");
    g_config_load_ok = config_load(g_config_path, &g_config);

    if (WSAStartup(MAKEWORD(2, 2), &winsock) != 0) {
        MessageBoxA(NULL, "Windows networking could not be initialized.", APP_TITLE, MB_OK | MB_ICONERROR);
        return 1;
    }

    common_controls.dwSize = sizeof(common_controls);
    common_controls.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&common_controls);

    memset(&metrics, 0, sizeof(metrics));
    metrics.cbSize = sizeof(metrics);
    if (SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0)) {
        metrics.lfMessageFont.lfHeight = scale_px(-15);
        strcpy(metrics.lfMessageFont.lfFaceName, "Segoe UI");
        g_font = CreateFontIndirectA(&metrics.lfMessageFont);
    } else {
        g_font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    }
    set_theme_palette(0);

    memset(&window_class, 0, sizeof(window_class));
    window_class.cbSize = sizeof(window_class);
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = window_proc;
    window_class.hInstance = instance;
    window_class.hIcon = LoadIconA(instance, MAKEINTRESOURCEA(IDI_APP_ICON));
    if (!window_class.hIcon) window_class.hIcon = LoadIconA(NULL, IDI_APPLICATION);
    window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    window_class.hbrBackground = g_background_brush;
    window_class.lpszClassName = "NICEquipmentManagerWindow";
    window_class.hIconSm = (HICON)LoadImageA(
        instance,
        MAKEINTRESOURCEA(IDI_APP_ICON),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON),
        LR_DEFAULTCOLOR
    );
    if (!window_class.hIconSm) window_class.hIconSm = window_class.hIcon;

    if (!RegisterClassExA(&window_class)) {
        MessageBoxA(NULL, "The application window could not be registered.", APP_TITLE, MB_OK | MB_ICONERROR);
        WSACleanup();
        return 1;
    }

    {
        WNDCLASSEXA advanced_class = window_class;
        advanced_class.lpfnWndProc = advanced_window_proc;
        advanced_class.lpszClassName = "NICEquipmentManagerAdvancedWindow";
        if (!RegisterClassExA(&advanced_class)) {
            MessageBoxA(NULL, "The Advanced Tools window could not be registered.", APP_TITLE, MB_OK | MB_ICONERROR);
            WSACleanup();
            return 1;
        }
    }

    {
        POINT minimum_size;
        get_main_min_track_size(NULL, &minimum_size);
        initial_width = minimum_size.x;
        initial_height = minimum_size.y;
    }
    if (saved_window_is_visible()) {
        initial_x = g_config.window_x;
        initial_y = g_config.window_y;
        initial_width = g_config.window_width;
        initial_height = g_config.window_height;
    }
    main_menu = create_main_menu();
    g_main = CreateWindowExA(
        0,
        window_class.lpszClassName,
        APP_TITLE " - Portable IPv4 Tool",
        WS_OVERLAPPEDWINDOW,
        initial_x,
        initial_y,
        initial_width,
        initial_height,
        NULL,
        main_menu,
        instance,
        NULL
    );
    if (!g_main) {
        MessageBoxA(NULL, "The application window could not be created.", APP_TITLE, MB_OK | MB_ICONERROR);
        WSACleanup();
        return 1;
    }

    ShowWindow(g_main, g_config.window_maximized ? SW_SHOWMAXIMIZED : show_command);
    UpdateWindow(g_main);
    while (GetMessage(&message, NULL, 0, 0) > 0) {
        if (!IsDialogMessage(g_main, &message)) {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
    }

    if (g_font && g_font != GetStockObject(DEFAULT_GUI_FONT)) DeleteObject(g_font);
    if (g_background_brush) DeleteObject(g_background_brush);
    if (g_edit_brush) DeleteObject(g_edit_brush);
    if (g_button_brush) DeleteObject(g_button_brush);
    if (g_button_pressed_brush) DeleteObject(g_button_pressed_brush);
    WSACleanup();
    return (int)message.wParam;
}
