#ifndef NIC_EQUIPMENT_MANAGER_VALIDATION_H
#define NIC_EQUIPMENT_MANAGER_VALIDATION_H

int ipv4_is_valid(const char *value);
int ipv4_parse_prefix(const char *value, int *prefix_out);
void string_trim(char *value);

#endif
