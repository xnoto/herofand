#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sysfs.h"

bool herofand_path_exists(const char *path) {
    return path != NULL && access(path, F_OK) == 0;
}

bool herofand_read_string(const char *path, char *buffer, size_t buffer_size) {
    FILE *file;
    size_t len;

    if (path == NULL || buffer == NULL || buffer_size == 0U) {
        return false;
    }

    file = fopen(path, "r");
    if (file == NULL) {
        return false;
    }

    if (fgets(buffer, (int)buffer_size, file) == NULL) {
        fclose(file);
        return false;
    }

    fclose(file);

    len = strlen(buffer);
    while (len > 0U && (buffer[len - 1U] == '\n' || buffer[len - 1U] == '\r')) {
        buffer[len - 1U] = '\0';
        --len;
    }

    return true;
}

bool herofand_read_int(const char *path, int *value) {
    char buffer[64];
    char *end;
    long parsed;

    if (value == NULL || !herofand_read_string(path, buffer, sizeof(buffer))) {
        return false;
    }

    errno = 0;
    parsed = strtol(buffer, &end, 10);
    if (errno != 0 || end == buffer || *end != '\0') {
        return false;
    }
    if (parsed < INT_MIN || parsed > INT_MAX) {
        return false;
    }

    *value = (int)parsed;
    return true;
}

bool herofand_write_int(const char *path, int value) {
    FILE *file;

    if (path == NULL) {
        return false;
    }

    file = fopen(path, "w");
    if (file == NULL) {
        return false;
    }

    if (fprintf(file, "%d\n", value) < 0) {
        fclose(file);
        return false;
    }

    if (fclose(file) != 0) {
        return false;
    }

    return true;
}

char *herofand_path_join(const char *left, const char *right) {
    int written;
    size_t size;
    char *path;

    if (left == NULL || right == NULL) {
        return NULL;
    }

    size = strlen(left) + strlen(right) + 2U;
    path = malloc(size);
    if (path == NULL) {
        return NULL;
    }

    written = snprintf(path, size, "%s/%s", left, right);
    if (written < 0 || (size_t)written >= size) {
        free(path);
        return NULL;
    }

    return path;
}
