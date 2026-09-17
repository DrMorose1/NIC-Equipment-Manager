#ifndef NIC_EQUIPMENT_MANAGER_CONFIG_H
#define NIC_EQUIPMENT_MANAGER_CONFIG_H

#define MAX_DEVICES 100
#define MAX_PROFILES 50
#define MAX_SECONDARY 16

typedef struct {
    char name[128];
    char address[32];
    int prefix;
    char protocol[8];
    int port;
} DeviceEntry;

typedef struct {
    char name[128];
    char adapter_id[128];
    int dhcp;
    char primary_ip[32];
    int prefix;
    char gateway[32];
    char dns1[32];
    char dns2[32];
    char secondary_ip[MAX_SECONDARY][32];
    int secondary_prefix[MAX_SECONDARY];
    int secondary_count;
} NetworkProfile;

typedef struct {
    char theme[16];
    int window_saved;
    int window_x;
    int window_y;
    int window_width;
    int window_height;
    int window_maximized;
    DeviceEntry devices[MAX_DEVICES];
    int device_count;
    NetworkProfile profiles[MAX_PROFILES];
    int profile_count;
} AppConfig;

int config_load(const char *path, AppConfig *config);
int config_save(const char *path, const AppConfig *config);

#endif
