#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../src/network_commands.h"

int main(void) {
    char out[2048];
    assert(network_build_static_reset_preamble(out, sizeof(out)) == 1);
    assert(strstr(out, "Set-NetIPInterface -InterfaceIndex $i -AddressFamily IPv4 -Dhcp Disabled") != NULL);
    assert(strstr(out, "Get-NetIPAddress -InterfaceIndex $i -AddressFamily IPv4") != NULL);
    assert(strstr(out, "Remove-NetIPAddress -Confirm:$false") != NULL);
    assert(strstr(out, "PrefixOrigin -eq 'Manual'") == NULL);
    assert(strstr(out, "Get-NetRoute -InterfaceIndex $i -AddressFamily IPv4 -DestinationPrefix '0.0.0.0/0'") != NULL);
    assert(strstr(out, "Remove-NetRoute -Confirm:$false") != NULL);
    {
        char cmd[1024];
        assert(network_build_netsh_static_primary(cmd, sizeof(cmd), "192.168.50.10", 24, "192.168.50.1") == 1);
        assert(strstr(cmd, "netsh interface ipv4 set address") != NULL);
        assert(strstr(cmd, "name=$i") != NULL);
        assert(strstr(cmd, "source=static") != NULL);
        assert(strstr(cmd, "address=192.168.50.10/24") != NULL);
        assert(strstr(cmd, "gateway=192.168.50.1") != NULL);
        assert(strstr(cmd, "store=persistent") != NULL);
    }
    {
        char cmd[1024];
        assert(network_build_netsh_static_primary(cmd, sizeof(cmd), "169.254.40.43", 16, "") == 1);
        assert(strstr(cmd, "address=169.254.40.43/16") != NULL);
        assert(strstr(cmd, "gateway=none") != NULL);
    }
    {
        char cmd[1024];
        assert(network_build_netsh_static_secondary(cmd, sizeof(cmd), "192.168.8.10", 16) == 1);
        assert(strstr(cmd, "netsh interface ipv4 add address") != NULL);
        assert(strstr(cmd, "address=192.168.8.10/16") != NULL);
        assert(strstr(cmd, "store=persistent") != NULL);
    }

    {
        char cmd[2048];
        assert(network_build_persistent_dhcp(cmd, sizeof(cmd), 0) == 1);
        assert(strstr(cmd, "Get-NetAdapter -InterfaceIndex $i") != NULL);
        assert(strstr(cmd, "EnableDHCP") != NULL);
        assert(strstr(cmd, "-Value 0") != NULL);
    }
    {
        char cmd[2048];
        assert(network_build_persistent_dhcp(cmd, sizeof(cmd), 1) == 1);
        assert(strstr(cmd, "EnableDHCP") != NULL);
        assert(strstr(cmd, "-Value 1") != NULL);
    }
    {
        char cmd[2048];
        assert(network_build_static_verification(cmd, sizeof(cmd), "192.168.20.254", 16) == 1);
        assert(strstr(cmd, "Dhcp") != NULL);
        assert(strstr(cmd, "Disabled") != NULL);
        assert(strstr(cmd, "192.168.20.254") != NULL);
        assert(strstr(cmd, "PrefixLength") != NULL);
    }
    {
        char cmd[2048];
        assert(network_build_dhcp_verification(cmd, sizeof(cmd)) == 1);
        assert(strstr(cmd, "Dhcp") != NULL);
        assert(strstr(cmd, "Enabled") != NULL);
    }
    puts("network command tests passed");
    return 0;
}
