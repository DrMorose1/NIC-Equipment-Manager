#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shell_tools.h"

static int has_context_name(const char *entry) {
    static const char *names[] = {"NIC_INDEX=", "NIC_NAME=", "DEVICE_IP=", "DEVICE_PREFIX="};
    size_t i;
    for (i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
        size_t length = strlen(names[i]);
        if (_strnicmp(entry, names[i], length) == 0) return 1;
    }
    return 0;
}

static int compare_environment_entries(const void *left, const void *right) {
    const char *const *a = (const char *const *)left;
    const char *const *b = (const char *const *)right;
    return _stricmp(*a, *b);
}

static char *build_environment(const ShellContext *context) {
    LPCH system_environment = GetEnvironmentStringsA();
    const char *entry;
    char nic_index[64];
    char nic_name[384];
    char device_ip[96];
    char device_prefix[64];
    const char **entries;
    size_t entry_count = 0;
    size_t entry_capacity = 4;
    size_t bytes = 1;
    size_t i;
    char *environment;
    char *cursor;

    if (!system_environment) return NULL;
    snprintf(nic_index, sizeof(nic_index), "NIC_INDEX=%lu", context ? context->nic_index : 0UL);
    snprintf(nic_name, sizeof(nic_name), "NIC_NAME=%s", context && context->nic_name ? context->nic_name : "");
    snprintf(device_ip, sizeof(device_ip), "DEVICE_IP=%s", context && context->device_ip ? context->device_ip : "");
    snprintf(device_prefix, sizeof(device_prefix), "DEVICE_PREFIX=%d", context ? context->device_prefix : 0);
    for (entry = system_environment; *entry; entry += strlen(entry) + 1) {
        if (!has_context_name(entry)) ++entry_capacity;
    }
    entries = (const char **)malloc(entry_capacity * sizeof(*entries));
    if (!entries) {
        FreeEnvironmentStringsA(system_environment);
        return NULL;
    }
    for (entry = system_environment; *entry; entry += strlen(entry) + 1) {
        if (!has_context_name(entry)) entries[entry_count++] = entry;
    }
    entries[entry_count++] = nic_index;
    entries[entry_count++] = nic_name;
    entries[entry_count++] = device_ip;
    entries[entry_count++] = device_prefix;
    qsort(entries, entry_count, sizeof(*entries), compare_environment_entries);
    for (i = 0; i < entry_count; ++i) bytes += strlen(entries[i]) + 1;
    environment = (char *)malloc(bytes);
    if (!environment) {
        free(entries);
        FreeEnvironmentStringsA(system_environment);
        return NULL;
    }
    cursor = environment;
    for (i = 0; i < entry_count; ++i) {
        size_t length = strlen(entries[i]) + 1;
        memcpy(cursor, entries[i], length);
        cursor += length;
    }
    *cursor = '\0';
    free(entries);
    FreeEnvironmentStringsA(system_environment);
    return environment;
}

int shell_tools_launch(HWND owner, ShellTool tool, const ShellContext *context, DWORD *error_out) {
    STARTUPINFOA startup;
    PROCESS_INFORMATION process;
    char command[256];
    char *environment = build_environment(context);
    BOOL created;
    (void)owner;

    if (error_out) *error_out = ERROR_SUCCESS;
    if (!environment) {
        if (error_out) *error_out = ERROR_NOT_ENOUGH_MEMORY;
        return 0;
    }
    strcpy(
        command,
        tool == SHELL_TOOL_POWERSHELL
            ? "powershell.exe -NoLogo -NoExit"
            : "cmd.exe /K title NIC Equipment Manager - Administrator Command Prompt"
    );
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    memset(&process, 0, sizeof(process));
    created = CreateProcessA(
        NULL,
        command,
        NULL,
        NULL,
        FALSE,
        CREATE_NEW_CONSOLE,
        environment,
        NULL,
        &startup,
        &process
    );
    if (!created) {
        if (error_out) *error_out = GetLastError();
        free(environment);
        return 0;
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    free(environment);
    return 1;
}

int shell_tools_copy_text(HWND owner, const char *text) {
    HGLOBAL memory;
    char *destination;
    size_t length = strlen(text ? text : "") + 1;
    if (!OpenClipboard(owner)) return 0;
    EmptyClipboard();
    memory = GlobalAlloc(GMEM_MOVEABLE, length);
    if (!memory) {
        CloseClipboard();
        return 0;
    }
    destination = (char *)GlobalLock(memory);
    memcpy(destination, text ? text : "", length);
    GlobalUnlock(memory);
    if (!SetClipboardData(CF_TEXT, memory)) {
        GlobalFree(memory);
        CloseClipboard();
        return 0;
    }
    CloseClipboard();
    return 1;
}
