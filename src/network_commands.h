#ifndef NETWORK_COMMANDS_H
#define NETWORK_COMMANDS_H

#include <stddef.h>

int network_build_static_reset_preamble(char *output, size_t size);
int network_build_persistent_dhcp(char *output, size_t size, int enabled);
int network_build_static_verification(char *output, size_t size, const char *ip, int prefix);
int network_build_dhcp_verification(char *output, size_t size);
int network_build_netsh_static_primary(char *output, size_t size, const char *ip, int prefix, const char *gateway);
int network_build_netsh_static_secondary(char *output, size_t size, const char *ip, int prefix);

#endif
