#ifndef _ARG_H_
#define _ARG_H_

typedef enum {
    ARG_STATUS_OK,
    ARG_STATUS_ESYNTAX,
    ARG_STATUS_ENOMEM,
} ArgStatusCode;

typedef ArgStatusCode (*ArgHandlerFn)(int argc, char** argv, int* i, void* data);

typedef struct {
    char arg_short;
    char* arg_long;
    ArgHandlerFn arg_fn;
} ArgDefinition;

typedef struct {
    ArgStatusCode status;
    int argc;
    char** argv;
} ArgParseResult;

ArgParseResult arg_parse(int argc, char** argv, ArgDefinition defs[], size_t def_count, void* data);

#endif
