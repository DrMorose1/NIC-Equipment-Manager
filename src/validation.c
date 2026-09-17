#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdlib.h>
#include <string.h>

#include "validation.h"

void string_trim(char *value) {
    size_t length;
    char *start = value;
    while (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n') ++start;
    if (start != value) memmove(value, start, strlen(start) + 1);
    length = strlen(value);
    while (length > 0) {
        char ch = value[length - 1];
        if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n') break;
        value[--length] = '\0';
    }
}

int ipv4_is_valid(const char *value) {
    struct in_addr address;
    return value && value[0] && InetPtonA(AF_INET, value, &address) == 1;
}

int ipv4_parse_prefix(const char *value, int *prefix_out) {
    char *end;
    long prefix;
    int i;
    int bit;
    int seen_zero = 0;
    unsigned char bytes[4];
    struct in_addr mask;

    if (!value || !value[0] || !prefix_out) return 0;
    prefix = strtol(value, &end, 10);
    if (*end == '\0' && prefix >= 1 && prefix <= 32) {
        *prefix_out = (int)prefix;
        return 1;
    }
    if (InetPtonA(AF_INET, value, &mask) != 1) return 0;
    memcpy(bytes, &mask, sizeof(bytes));
    prefix = 0;
    for (i = 0; i < 4; ++i) {
        for (bit = 7; bit >= 0; --bit) {
            if (bytes[i] & (1 << bit)) {
                if (seen_zero) return 0;
                ++prefix;
            } else {
                seen_zero = 1;
            }
        }
    }
    if (prefix < 1 || prefix > 32) return 0;
    *prefix_out = (int)prefix;
    return 1;
}
