#ifndef HEROFAND_SYSFS_H
#define HEROFAND_SYSFS_H

#include <stdbool.h>
#include <stddef.h>

bool herofand_path_exists(const char *path);
bool herofand_read_string(const char *path, char *buffer, size_t buffer_size);
bool herofand_read_int(const char *path, int *value);
bool herofand_write_int(const char *path, int value);
char *herofand_path_join(const char *left, const char *right);

#endif
