#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <stdarg.h>

int io_confirm(const char* fmt, ...) {
    char* line = NULL;
    int result = 0;
    va_list arg;
    va_start(arg, fmt);
    size_t len = 0;
    ssize_t l = -1;

    vfprintf(stdout, fmt, arg);
    while ((l = getline(&line, &len, stdin)) != -1) {
        if (strncasecmp(line, "y", 1) == 0) {
            result = 1;
            break;
        }
        if (strncasecmp(line, "n", 1) == 0) {
            break;
        }
        free(line);
        line = NULL;
        vfprintf(stdout, fmt, arg);
    }

    free(line);
    va_end(arg);
    return result;
}
