#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../src/diagnostics.h"

static void test_disconnected_speed_is_unknown(void) {
    char out[64];
    diagnostics_format_link_speed(0, UINT64_MAX, out, sizeof(out));
    assert(strcmp(out, "Unknown") == 0);
}

static void test_connected_speed_formats_normally(void) {
    char out[64];
    diagnostics_format_link_speed(1, 1000000000ULL, out, sizeof(out));
    assert(strcmp(out, "1.0 Gbps") == 0);
}

static void test_sentinel_or_impossible_speed_is_unknown(void) {
    char out[64];
    diagnostics_format_link_speed(1, UINT64_MAX, out, sizeof(out));
    assert(strcmp(out, "Unknown") == 0);
}

static void test_targeted_commands_require_and_quote_target(void) {
    char out[256];
    assert(diagnostics_build_command(DIAG_TRACERT, "10.0.0.1", out, sizeof(out)) == 1);
    assert(strcmp(out, "tracert -d 10.0.0.1") == 0);
    assert(diagnostics_build_command(DIAG_PATHPING, "", out, sizeof(out)) == 0);
    assert(diagnostics_build_command(DIAG_NSLOOKUP, "10.0.0.1", out, sizeof(out)) == 1);
    assert(strcmp(out, "nslookup 10.0.0.1") == 0);
}

static void test_local_commands_do_not_need_target(void) {
    char out[256];
    assert(diagnostics_build_command(DIAG_IPCONFIG_ALL, NULL, out, sizeof(out)) == 1);
    assert(strcmp(out, "ipconfig /all") == 0);
    assert(diagnostics_build_command(DIAG_ROUTE_PRINT, NULL, out, sizeof(out)) == 1);
    assert(strcmp(out, "route print -4") == 0);
    assert(diagnostics_build_command(DIAG_ARP, NULL, out, sizeof(out)) == 1);
    assert(strcmp(out, "arp -a") == 0);
    assert(diagnostics_build_command(DIAG_NETSTAT, NULL, out, sizeof(out)) == 1);
    assert(strcmp(out, "netstat -ano") == 0);
}

int main(void) {
    test_disconnected_speed_is_unknown();
    test_connected_speed_formats_normally();
    test_sentinel_or_impossible_speed_is_unknown();
    test_targeted_commands_require_and_quote_target();
    test_local_commands_do_not_need_target();
    puts("diagnostics tests passed");
    return 0;
}
