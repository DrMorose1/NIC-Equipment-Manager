#include <stdio.h>
#include <string.h>

#include "../src/config.h"

int main(void) {
    const char *path = "/tmp/nic-equipment-manager-config-test.json";
    const char *legacy_path = "/tmp/nic-equipment-manager-legacy-config-test.json";
    AppConfig original;
    AppConfig loaded;
    FILE *legacy;
    memset(&original, 0, sizeof(original));
    strcpy(original.theme, "dark");
    original.window_saved = 1;
    original.window_x = 120;
    original.window_y = 80;
    original.window_width = 1440;
    original.window_height = 900;
    original.window_maximized = 1;

    original.device_count = 1;
    strcpy(original.devices[0].name, "Lab \"Switch\"");
    strcpy(original.devices[0].address, "192.168.50.2");
    original.devices[0].prefix = 24;
    strcpy(original.devices[0].protocol, "https");
    original.devices[0].port = 8443;

    original.profile_count = 1;
    strcpy(original.profiles[0].name, "Bench Profile");
    strcpy(original.profiles[0].adapter_id, "{TEST-ADAPTER}");
    original.profiles[0].dhcp = 0;
    strcpy(original.profiles[0].primary_ip, "192.168.50.10");
    original.profiles[0].prefix = 24;
    strcpy(original.profiles[0].dns1, "1.1.1.1");
    strcpy(original.profiles[0].secondary_ip[0], "10.10.10.10");
    original.profiles[0].secondary_prefix[0] = 24;
    original.profiles[0].secondary_count = 1;

    if (!config_save(path, &original)) return 1;
    if (!config_load(path, &loaded)) return 2;
    if (loaded.device_count != 1 || loaded.profile_count != 1) return 3;
    if (strcmp(loaded.theme, "dark") != 0) return 4;
    if (strcmp(loaded.devices[0].name, original.devices[0].name) != 0) return 5;
    if (strcmp(loaded.devices[0].address, "192.168.50.2") != 0) return 6;
    if (loaded.devices[0].port != 8443) return 7;
    if (loaded.devices[0].prefix != 24) return 8;
    if (loaded.profiles[0].secondary_count != 1) return 9;
    if (strcmp(loaded.profiles[0].secondary_ip[0], "10.10.10.10") != 0) return 10;
    if (loaded.profiles[0].secondary_prefix[0] != 24) return 11;
    if (!loaded.window_saved || loaded.window_x != 120 || loaded.window_y != 80) return 16;
    if (loaded.window_width != 1440 || loaded.window_height != 900) return 17;
    if (!loaded.window_maximized) return 18;

    legacy = fopen(legacy_path, "wb");
    if (!legacy) return 12;
    fputs("{\"devices\":[{\"name\":\"Old Device\",\"address\":\"10.0.0.2\","
          "\"protocol\":\"http\",\"port\":0}],\"profiles\":[]}", legacy);
    fclose(legacy);
    if (!config_load(legacy_path, &loaded)) return 13;
    if (loaded.device_count != 1 || loaded.devices[0].prefix != 24) return 14;
    if (strcmp(loaded.theme, "system") != 0) return 15;
    if (loaded.window_saved) return 19;

    puts("config round-trip passed");
    return 0;
}
