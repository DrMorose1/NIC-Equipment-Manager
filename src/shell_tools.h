#ifndef NIC_EQUIPMENT_MANAGER_SHELL_TOOLS_H
#define NIC_EQUIPMENT_MANAGER_SHELL_TOOLS_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef enum {
    SHELL_TOOL_POWERSHELL,
    SHELL_TOOL_COMMAND_PROMPT
} ShellTool;

typedef struct {
    unsigned long nic_index;
    const char *nic_name;
    const char *device_ip;
    int device_prefix;
} ShellContext;

int shell_tools_launch(HWND owner, ShellTool tool, const ShellContext *context, DWORD *error_out);
int shell_tools_copy_text(HWND owner, const char *text);

#endif
