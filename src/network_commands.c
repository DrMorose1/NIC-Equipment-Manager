#include "network_commands.h"

#include <stdio.h>


int network_build_persistent_dhcp(char *output, size_t size, int enabled) {
    int written;
    if (!output || size == 0) return 0;
    written = snprintf(
        output,
        size,
        "$a=Get-NetAdapter -InterfaceIndex $i -ErrorAction Stop; "
        "$p='HKLM:\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\'+$a.InterfaceGuid; "
        "Set-ItemProperty -Path $p -Name EnableDHCP -Value %d -Type DWord -ErrorAction Stop; ",
        enabled ? 1 : 0
    );
    return written > 0 && (size_t)written < size;
}

int network_build_static_verification(char *output, size_t size, const char *ip, int prefix) {
    int written;
    if (!output || size == 0 || !ip || !ip[0] || prefix < 0 || prefix > 32) return 0;
    written = snprintf(
        output,
        size,
        "$vif=Get-NetIPInterface -InterfaceIndex $i -AddressFamily IPv4 -ErrorAction Stop; "
        "$vip=Get-NetIPAddress -InterfaceIndex $i -AddressFamily IPv4 -IPAddress '%s' -ErrorAction SilentlyContinue | "
        "Where-Object {$_.PrefixLength -eq %d} | Select-Object -First 1; "
        "if ($vif.Dhcp -ne 'Disabled' -or -not $vip) { throw 'Windows did not retain the requested static IPv4 configuration.' }; ",
        ip,
        prefix
    );
    return written > 0 && (size_t)written < size;
}

int network_build_dhcp_verification(char *output, size_t size) {
    int written;
    if (!output || size == 0) return 0;
    written = snprintf(
        output,
        size,
        "$vif=Get-NetIPInterface -InterfaceIndex $i -AddressFamily IPv4 -ErrorAction Stop; "
        "if ($vif.Dhcp -ne 'Enabled') { throw 'Windows did not retain DHCP mode on the selected interface.' }; "
    );
    return written > 0 && (size_t)written < size;
}

int network_build_static_reset_preamble(char *output, size_t size) {
    int written;
    if (!output || size == 0) return 0;
    written = snprintf(
        output,
        size,
        "Set-NetIPInterface -InterfaceIndex $i -AddressFamily IPv4 -Dhcp Disabled -ErrorAction Stop; "
        "Get-NetRoute -InterfaceIndex $i -AddressFamily IPv4 -DestinationPrefix '0.0.0.0/0' -ErrorAction SilentlyContinue | "
        "Remove-NetRoute -Confirm:$false -ErrorAction SilentlyContinue; "
        "Get-NetIPAddress -InterfaceIndex $i -AddressFamily IPv4 -ErrorAction SilentlyContinue | "
        "Remove-NetIPAddress -Confirm:$false -ErrorAction SilentlyContinue; "
    );
    return written > 0 && (size_t)written < size;
}


int network_build_netsh_static_primary(char *output, size_t size, const char *ip, int prefix, const char *gateway) {
    int written;
    const char *gw = (gateway && gateway[0]) ? gateway : "none";
    if (!output || size == 0 || !ip || !ip[0] || prefix < 0 || prefix > 32) return 0;
    written = snprintf(
        output,
        size,
        "& netsh interface ipv4 set address name=$i source=static address=%s/%d gateway=%s store=persistent | Out-Null; "
        "if ($LASTEXITCODE -ne 0) { throw 'netsh failed while setting the primary IPv4 address.' }; ",
        ip,
        prefix,
        gw
    );
    return written > 0 && (size_t)written < size;
}

int network_build_netsh_static_secondary(char *output, size_t size, const char *ip, int prefix) {
    int written;
    if (!output || size == 0 || !ip || !ip[0] || prefix < 0 || prefix > 32) return 0;
    written = snprintf(
        output,
        size,
        "& netsh interface ipv4 add address name=$i address=%s/%d store=persistent | Out-Null; "
        "if ($LASTEXITCODE -ne 0) { throw 'netsh failed while adding a secondary IPv4 address.' }; ",
        ip,
        prefix
    );
    return written > 0 && (size_t)written < size;
}
