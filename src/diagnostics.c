#include "diagnostics.h"

#include <stdio.h>
#include <string.h>

#define MAX_REASONABLE_LINK_SPEED_BPS 10000000000000ULL

void diagnostics_format_link_speed(int connected, uint64_t bits_per_second, char *output, size_t size) {
    if (!output || size == 0) return;
    output[0] = '\0';
    if (!connected || bits_per_second == 0 || bits_per_second > MAX_REASONABLE_LINK_SPEED_BPS) {
        snprintf(output, size, "Unknown");
    } else if (bits_per_second >= 1000000000ULL) {
        snprintf(output, size, "%.1f Gbps", (double)bits_per_second / 1000000000.0);
    } else if (bits_per_second >= 1000000ULL) {
        snprintf(output, size, "%.0f Mbps", (double)bits_per_second / 1000000.0);
    } else if (bits_per_second >= 1000ULL) {
        snprintf(output, size, "%.0f Kbps", (double)bits_per_second / 1000.0);
    } else {
        snprintf(output, size, "%llu bps", (unsigned long long)bits_per_second);
    }
}

int diagnostics_build_command(DiagnosticCommand command, const char *target, char *output, size_t size) {
    const char *value = target ? target : "";
    if (!output || size == 0) return 0;
    output[0] = '\0';
    switch (command) {
        case DIAG_IPCONFIG_ALL:
            snprintf(output, size, "ipconfig /all");
            return 1;
        case DIAG_ROUTE_PRINT:
            snprintf(output, size, "route print -4");
            return 1;
        case DIAG_ARP:
            snprintf(output, size, "arp -a");
            return 1;
        case DIAG_NETSTAT:
            snprintf(output, size, "netstat -ano");
            return 1;
        case DIAG_TRACERT:
            if (!value[0]) return 0;
            snprintf(output, size, "tracert -d %s", value);
            return 1;
        case DIAG_PATHPING:
            if (!value[0]) return 0;
            snprintf(output, size, "pathping -n %s", value);
            return 1;
        case DIAG_NSLOOKUP:
            if (!value[0]) return 0;
            snprintf(output, size, "nslookup %s", value);
            return 1;
        default:
            return 0;
    }
}
