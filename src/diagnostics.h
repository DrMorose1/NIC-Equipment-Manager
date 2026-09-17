#ifndef NIC_EQUIPMENT_MANAGER_DIAGNOSTICS_H
#define NIC_EQUIPMENT_MANAGER_DIAGNOSTICS_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    DIAG_IPCONFIG_ALL,
    DIAG_ROUTE_PRINT,
    DIAG_ARP,
    DIAG_NETSTAT,
    DIAG_TRACERT,
    DIAG_PATHPING,
    DIAG_NSLOOKUP
} DiagnosticCommand;

void diagnostics_format_link_speed(int connected, uint64_t bits_per_second, char *output, size_t size);
int diagnostics_build_command(DiagnosticCommand command, const char *target, char *output, size_t size);

#endif
