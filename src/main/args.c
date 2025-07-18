#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "args.h"

ArgParseResult arg_parse(int argc, char** argv, size_t defc, ArgDefinition defv[], void* data) {
    ArgParseResult result = {
        .status = ARG_STATUS_ESYNTAX,
        .argc = 0,
        .argv = NULL,
    };

    result.argv = malloc(argc * sizeof(char*));
    if (!result.argv) {
        result.status = ARG_STATUS_ENOMEM;
        goto error;
    }

    int new_argc = 0;
    int i = 0;

    for (i = 0; i < argc; i++) {
        char* arg = argv[i];

        // long option
        if (strncmp("--", arg, 2) == 0) {
            // break loop if "--"
            if (!arg[2]) {
                i++;
                break;
            }

            int found = 0;
            for (size_t j = 0; !found && j < defc; j++) {
                ArgDefinition a = defv[j];
                if (strcmp(a.arg_long, arg+2) == 0) {
                    if (a.arg_fn(argc, argv, &i, data) != ARG_STATUS_OK) goto error;
                    found = 1;
                }
            }
            if (found) continue;
            fprintf(stderr, "Invalid argument: %s\n", arg);
            goto error;
        }

        // short option
        if (strncmp("-", arg, 1) == 0) {
            int found = 0;
            if (strlen(arg) == 2) {
                for (size_t j = 0; !found && j < defc; j++) {
                    ArgDefinition a = defv[j];
                    if (a.arg_short == arg[1]) {
                        if (a.arg_fn(argc, argv, &i, data) != ARG_STATUS_OK) goto error;
                        found = 1;
                    }
                }
            }
            if (found) continue;
            fprintf(stderr, "Invalid argument: %s\n", arg);
            goto error;
        }

        // positional argument
        result.argv[new_argc++] = arg;
    }

    while (i < argc) result.argv[new_argc++] = argv[i++];

    result.argc = new_argc;
    result.status = ARG_STATUS_OK;
    return result;

error:
    free(result.argv);
    result.argv = NULL;
    return result;
}

