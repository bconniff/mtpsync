#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <libgen.h>

#include "../main/array.h"
#include "../main/args.h"

typedef struct {
    int option;
    char* string;
} TestArgs;

static ArgStatusCode option_arg(int argc, char** argv, int* i, void* data) {
    TestArgs* args = data;
    args->option = 1;
    return ARG_STATUS_OK;
}

static ArgStatusCode string_arg(int argc, char** argv, int* i, void* data) {
    TestArgs* args = data;
    args->string = argv[++(*i)];
    return ARG_STATUS_OK;
}

int args_test_short() {
    char* argv[] = { "one", "-o", "two", "-s", "test", "three", "--", "-s" };
    TestArgs args = {0};

    ArgDefinition defv[] = {
        { .arg_long = "option", .arg_short = 'o', .arg_fn = option_arg },
        { .arg_long = "string", .arg_short = 's', .arg_fn = string_arg },
    };

    size_t argc = ARRAY_LEN(argv);
    size_t defc = ARRAY_LEN(defv);
    ArgParseResult result = arg_parse(argc, argv, defc, defv, &args);

    assert(strcmp("test", args.string) == 0);
    assert(args.option);
    assert(result.argc == 4);
    assert(strcmp("one", result.argv[0]) == 0);
    assert(strcmp("two", result.argv[1]) == 0);
    assert(strcmp("three", result.argv[2]) == 0);
    assert(strcmp("-s", result.argv[3]) == 0);

    assert(result.status == ARG_STATUS_OK);
    free(result.argv);

    return 0;
}

int args_test_long() {
    char* argv[] = { "one", "--option", "two", "--string", "test", "three", "--", "--string" };
    TestArgs args = {0};

    ArgDefinition defv[] = {
        { .arg_long = "option", .arg_short = 'o', .arg_fn = option_arg },
        { .arg_long = "string", .arg_short = 's', .arg_fn = string_arg },
    };

    size_t argc = ARRAY_LEN(argv);
    size_t defc = ARRAY_LEN(defv);
    ArgParseResult result = arg_parse(argc, argv, defc, defv, &args);

    assert(strcmp("test", args.string) == 0);
    assert(args.option);
    assert(result.argc == 4);
    assert(strcmp("one", result.argv[0]) == 0);
    assert(strcmp("two", result.argv[1]) == 0);
    assert(strcmp("three", result.argv[2]) == 0);
    assert(strcmp("--string", result.argv[3]) == 0);

    assert(result.status == ARG_STATUS_OK);
    free(result.argv);

    return 0;
}

int args_test() {
    args_test_short();
    args_test_long();
    return 0;
}
