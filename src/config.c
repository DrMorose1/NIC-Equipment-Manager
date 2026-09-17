#include "config.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void json_write_string(FILE *file, const char *value) {
    const unsigned char *p = (const unsigned char *)value;
    fputc('"', file);
    while (*p) {
        switch (*p) {
            case '"': fputs("\\\"", file); break;
            case '\\': fputs("\\\\", file); break;
            case '\b': fputs("\\b", file); break;
            case '\f': fputs("\\f", file); break;
            case '\n': fputs("\\n", file); break;
            case '\r': fputs("\\r", file); break;
            case '\t': fputs("\\t", file); break;
            default:
                if (*p < 0x20) {
                    fprintf(file, "\\u%04x", (unsigned int)*p);
                } else {
                    fputc(*p, file);
                }
        }
        ++p;
    }
    fputc('"', file);
}

int config_save(const char *path, const AppConfig *config) {
    int i;
    int j;
    FILE *file = fopen(path, "wb");
    if (!file) return 0;

    fputs("{\n  \"version\": 4,\n  \"theme\": ", file);
    json_write_string(file, config->theme[0] ? config->theme : "system");
    fprintf(
        file,
        ",\n  \"windowSaved\": %s,\n"
        "  \"windowX\": %d,\n"
        "  \"windowY\": %d,\n"
        "  \"windowWidth\": %d,\n"
        "  \"windowHeight\": %d,\n"
        "  \"windowMaximized\": %s,\n"
        "  \"devices\": [",
        config->window_saved ? "true" : "false",
        config->window_x,
        config->window_y,
        config->window_width,
        config->window_height,
        config->window_maximized ? "true" : "false"
    );
    for (i = 0; i < config->device_count; ++i) {
        const DeviceEntry *d = &config->devices[i];
        fputs(i == 0 ? "\n    {" : ",\n    {", file);
        fputs("\"name\": ", file); json_write_string(file, d->name);
        fputs(", \"address\": ", file); json_write_string(file, d->address);
        fprintf(file, ", \"prefix\": %d", d->prefix);
        fputs(", \"protocol\": ", file); json_write_string(file, d->protocol);
        fprintf(file, ", \"port\": %d}", d->port);
    }
    fputs(config->device_count ? "\n  ],\n  \"profiles\": [" : "],\n  \"profiles\": [", file);

    for (i = 0; i < config->profile_count; ++i) {
        const NetworkProfile *p = &config->profiles[i];
        fputs(i == 0 ? "\n    {\n" : ",\n    {\n", file);
        fputs("      \"name\": ", file); json_write_string(file, p->name);
        fputs(",\n      \"adapterId\": ", file); json_write_string(file, p->adapter_id);
        fprintf(file, ",\n      \"dhcp\": %s", p->dhcp ? "true" : "false");
        fputs(",\n      \"primaryIp\": ", file); json_write_string(file, p->primary_ip);
        fprintf(file, ", \"prefix\": %d", p->prefix);
        fputs(",\n      \"gateway\": ", file); json_write_string(file, p->gateway);
        fputs(", \"dns1\": ", file); json_write_string(file, p->dns1);
        fputs(", \"dns2\": ", file); json_write_string(file, p->dns2);
        fputs(",\n      \"secondary\": [", file);
        for (j = 0; j < p->secondary_count; ++j) {
            if (j) fputs(", ", file);
            fputc('"', file);
            fputs(p->secondary_ip[j], file);
            fprintf(file, "/%d\"", p->secondary_prefix[j]);
        }
        fputs("]\n    }", file);
    }
    fputs(config->profile_count ? "\n  ]\n}\n" : "]\n}\n", file);

    if (fclose(file) != 0) return 0;
    return 1;
}

static char *read_file(const char *path, size_t *size_out) {
    long length;
    size_t read_count;
    char *data;
    FILE *file = fopen(path, "rb");
    if (!file) return NULL;
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }
    length = ftell(file);
    if (length < 0 || length > 10 * 1024 * 1024) {
        fclose(file);
        return NULL;
    }
    rewind(file);
    data = (char *)malloc((size_t)length + 1);
    if (!data) {
        fclose(file);
        return NULL;
    }
    read_count = fread(data, 1, (size_t)length, file);
    fclose(file);
    if (read_count != (size_t)length) {
        free(data);
        return NULL;
    }
    data[length] = '\0';
    if (size_out) *size_out = (size_t)length;
    return data;
}

static const char *skip_ws(const char *p) {
    while (*p && isspace((unsigned char)*p)) ++p;
    return p;
}

static const char *find_key(const char *start, const char *end, const char *key) {
    char pattern[160];
    size_t length;
    const char *p = start;
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    length = strlen(pattern);
    while (p && p < end) {
        p = strstr(p, pattern);
        if (!p || p >= end) return NULL;
        p += length;
        p = skip_ws(p);
        if (p < end && *p == ':') return skip_ws(p + 1);
    }
    return NULL;
}

static int parse_json_string(const char *p, const char *end, char *out, size_t out_size) {
    size_t n = 0;
    if (!p || p >= end || *p != '"') return 0;
    ++p;
    while (p < end && *p && *p != '"') {
        unsigned char ch = (unsigned char)*p++;
        if (ch == '\\' && p < end) {
            ch = (unsigned char)*p++;
            switch (ch) {
                case '"': ch = '"'; break;
                case '\\': ch = '\\'; break;
                case '/': ch = '/'; break;
                case 'b': ch = '\b'; break;
                case 'f': ch = '\f'; break;
                case 'n': ch = '\n'; break;
                case 'r': ch = '\r'; break;
                case 't': ch = '\t'; break;
                case 'u':
                    if (p + 4 <= end) {
                        p += 4;
                        ch = '?';
                    }
                    break;
                default: break;
            }
        }
        if (n + 1 < out_size) out[n++] = (char)ch;
    }
    if (out_size) out[n] = '\0';
    return p < end && *p == '"';
}

static int get_string(const char *start, const char *end, const char *key, char *out, size_t out_size) {
    const char *p = find_key(start, end, key);
    if (!p) {
        if (out_size) out[0] = '\0';
        return 0;
    }
    return parse_json_string(p, end, out, out_size);
}

static int get_int(const char *start, const char *end, const char *key, int fallback) {
    const char *p = find_key(start, end, key);
    char *number_end;
    long value;
    if (!p) return fallback;
    value = strtol(p, &number_end, 10);
    if (number_end == p) return fallback;
    return (int)value;
}

static int get_bool(const char *start, const char *end, const char *key, int fallback) {
    const char *p = find_key(start, end, key);
    if (!p) return fallback;
    if (strncmp(p, "true", 4) == 0) return 1;
    if (strncmp(p, "false", 5) == 0) return 0;
    return fallback;
}

static const char *find_matching(const char *start, const char *limit, char open_ch, char close_ch) {
    int depth = 0;
    int in_string = 0;
    int escaped = 0;
    const char *p;
    for (p = start; p < limit && *p; ++p) {
        char ch = *p;
        if (in_string) {
            if (escaped) {
                escaped = 0;
            } else if (ch == '\\') {
                escaped = 1;
            } else if (ch == '"') {
                in_string = 0;
            }
            continue;
        }
        if (ch == '"') {
            in_string = 1;
        } else if (ch == open_ch) {
            ++depth;
        } else if (ch == close_ch) {
            --depth;
            if (depth == 0) return p;
        }
    }
    return NULL;
}

static int parse_secondary(const char *start, const char *end, NetworkProfile *profile) {
    const char *p = find_key(start, end, "secondary");
    if (!p || *p != '[') return 1;
    ++p;
    while (p < end && profile->secondary_count < MAX_SECONDARY) {
        char combined[64];
        char *slash;
        p = skip_ws(p);
        if (*p == ']') break;
        if (*p == ',') {
            ++p;
            continue;
        }
        if (*p != '"') return 0;
        if (!parse_json_string(p, end, combined, sizeof(combined))) return 0;
        p++;
        while (p < end) {
            if (*p == '\\' && p + 1 < end) {
                p += 2;
                continue;
            }
            if (*p++ == '"') break;
        }
        slash = strrchr(combined, '/');
        if (!slash) continue;
        *slash = '\0';
        if (strlen(combined) >= sizeof(profile->secondary_ip[profile->secondary_count])) return 0;
        memcpy(profile->secondary_ip[profile->secondary_count], combined, strlen(combined) + 1);
        profile->secondary_prefix[profile->secondary_count] = atoi(slash + 1);
        ++profile->secondary_count;
    }
    return 1;
}

static int parse_object_array(const char *data, const char *key, AppConfig *config, int devices) {
    const char *data_end = data + strlen(data);
    const char *array = find_key(data, data_end, key);
    const char *array_end;
    const char *p;
    if (!array || *array != '[') return 1;
    array_end = find_matching(array, data_end, '[', ']');
    if (!array_end) return 0;
    p = array + 1;
    while (p < array_end) {
        const char *obj_start = strchr(p, '{');
        const char *obj_end;
        if (!obj_start || obj_start >= array_end) break;
        obj_end = find_matching(obj_start, array_end + 1, '{', '}');
        if (!obj_end) return 0;
        if (devices) {
            DeviceEntry *d;
            if (config->device_count >= MAX_DEVICES) break;
            d = &config->devices[config->device_count];
            memset(d, 0, sizeof(*d));
            get_string(obj_start, obj_end, "name", d->name, sizeof(d->name));
            get_string(obj_start, obj_end, "address", d->address, sizeof(d->address));
            d->prefix = get_int(obj_start, obj_end, "prefix", 24);
            if (d->prefix < 1 || d->prefix > 32) d->prefix = 24;
            get_string(obj_start, obj_end, "protocol", d->protocol, sizeof(d->protocol));
            d->port = get_int(obj_start, obj_end, "port", 0);
            if (d->address[0]) ++config->device_count;
        } else {
            NetworkProfile *profile;
            if (config->profile_count >= MAX_PROFILES) break;
            profile = &config->profiles[config->profile_count];
            memset(profile, 0, sizeof(*profile));
            get_string(obj_start, obj_end, "name", profile->name, sizeof(profile->name));
            get_string(obj_start, obj_end, "adapterId", profile->adapter_id, sizeof(profile->adapter_id));
            profile->dhcp = get_bool(obj_start, obj_end, "dhcp", 0);
            get_string(obj_start, obj_end, "primaryIp", profile->primary_ip, sizeof(profile->primary_ip));
            profile->prefix = get_int(obj_start, obj_end, "prefix", 24);
            get_string(obj_start, obj_end, "gateway", profile->gateway, sizeof(profile->gateway));
            get_string(obj_start, obj_end, "dns1", profile->dns1, sizeof(profile->dns1));
            get_string(obj_start, obj_end, "dns2", profile->dns2, sizeof(profile->dns2));
            if (!parse_secondary(obj_start, obj_end, profile)) return 0;
            if (profile->name[0]) ++config->profile_count;
        }
        p = obj_end + 1;
    }
    return 1;
}

int config_load(const char *path, AppConfig *config) {
    size_t size = 0;
    char *data;
    memset(config, 0, sizeof(*config));
    strcpy(config->theme, "system");
    data = read_file(path, &size);
    if (!data) return 1;
    if (size == 0) {
        free(data);
        return 1;
    }
    if (!parse_object_array(data, "devices", config, 1) ||
        !parse_object_array(data, "profiles", config, 0)) {
        memset(config, 0, sizeof(*config));
        strcpy(config->theme, "system");
        free(data);
        return 0;
    }
    get_string(data, data + size, "theme", config->theme, sizeof(config->theme));
    if (strcmp(config->theme, "system") != 0 &&
        strcmp(config->theme, "light") != 0 &&
        strcmp(config->theme, "dark") != 0) {
        strcpy(config->theme, "system");
    }
    config->window_saved = get_bool(data, data + size, "windowSaved", 0);
    config->window_x = get_int(data, data + size, "windowX", 0);
    config->window_y = get_int(data, data + size, "windowY", 0);
    config->window_width = get_int(data, data + size, "windowWidth", 0);
    config->window_height = get_int(data, data + size, "windowHeight", 0);
    config->window_maximized = get_bool(data, data + size, "windowMaximized", 0);
    free(data);
    return 1;
}
