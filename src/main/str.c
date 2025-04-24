#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

#include "str.h"

#define STR_BUF_SIZE 128

char* str_join(size_t argc, ...) {
    char* result = NULL;

    size_t total_len = 0;
    size_t capacity = STR_BUF_SIZE;

    result = malloc(capacity * sizeof(char*));
    if (!result) goto error;

    va_list ap;
    va_start(ap, argc);

    for (size_t i = 0; i < argc; i++) {
        char* s = va_arg(ap, char*);
        size_t len = strlen(s);
        size_t new_len = len + total_len;

        if ((new_len + 1) > capacity) {
            while ((new_len + 1) > (capacity *= 2));
            char* new_result = realloc(result, capacity * sizeof(char*));
            if (!new_result) goto error;
            result = new_result;
        }

        strcpy(result + total_len, s);
        total_len += len;
    }

    va_end(ap);
    result[total_len] = 0;
    return result;

error:
    free(result);
    va_end(ap);
    return NULL;
}

int str_starts_with(char* str, char* prefix) {
    size_t strl = strlen(str);
    size_t prefixl = strlen(prefix);
    return strl >= prefixl && strncmp(str, prefix, prefixl) == 0;
}

int str_ends_with(char* str, char* suffix) {
    size_t strl = strlen(str);
    size_t suffixl = strlen(suffix);
    return strl >= suffixl && strncmp(str + strl - suffixl, suffix, suffixl) == 0;
}

char* str_lower(char* str) {
    char* dup = NULL;

    if (!str) return NULL;

    dup = strdup(str);
    if (!dup) return NULL;

    for (char* p = dup; *p; p++) *p = tolower(*p);

    return dup;
}

char* str_upper(char* str) {
    char* dup = NULL;

    if (!str) return NULL;

    dup = strdup(str);
    if (!dup) return NULL;

    for (char* p = dup; *p; p++) *p = toupper(*p);

    return dup;
}

inline size_t str_count_char(char* str, char ch) {
    size_t count = 0;
    while (*str) {
        if (*str++ == ch) {
            count++;
        }
    }
    return count;
}
