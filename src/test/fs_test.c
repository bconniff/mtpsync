#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <libgen.h>

#include "../main/str.h"
#include "../main/fs.h"
#include "../main/list.h"

static void assert_join(char* expected, char* a, char* b) {
    //printf("---------------------------\n");
    //printf("JOIN %s + %s\n", a, b); fflush(stdout);
    char* actual = fs_path_join(a, b);
    assert(actual);
    //printf("%s = %s\n", actual, expected); fflush(stdout);
    assert(strcmp(expected, actual) == 0);
    free(actual);
}

static void assert_resolve(char* expect, char* input) {
    //printf("---------------------------\n");
    //printf("RESOLVE %s\n", input); fflush(stdout);
    char* actual = fs_resolve(input);

    fflush(stdout);
    assert(actual);
    assert(strcmp(expect, actual) == 0);
    free(actual);
}

static void assert_resolve_rel(char* expect, char* input) {
    char* cwd = getcwd(NULL, 0);
    assert(cwd);
    char* fullpath = fs_path_join(cwd, expect);
    assert(fullpath);
    free(cwd);

    assert_resolve(fullpath, input);
    free(fullpath);
}

int fs_test(int sz) {
    // TEST COLLECT FILES
    List* files = fs_collect_files(".");
    assert(files);

    int found = 0;

    char* expect_file = fs_resolve("src/test/fs_test.c");
    assert(expect_file);

    for (size_t i = 0; i < list_size(files); i++) {
        if (strcmp(expect_file, list_get(files, i)) == 0) {
            found = 1;
            break;
        }
    }

    list_free_deep(files, free);

    assert(found);

    // TEST COLLECT A SINGLE FILE
    files = fs_collect_files("./src/test/fs_test.c");
    assert(files);
    assert(list_size(files) == 1);
    assert(strcmp(expect_file, (char*)list_get(files, 0)) == 0);
    list_free_deep(files, free);
    free(expect_file);

    // TEST COLLECT ANCESTORS
    char* cwd = getcwd(NULL, 0);
    assert(cwd);

    files = fs_collect_ancestors(cwd);
    assert(files);
    assert(list_size(files) >= 1);
    assert(strcmp("/", (char*)list_get(files, 0)) == 0);

    found = 0;
    for (size_t i = 0; i < list_size(files); i++) {
        if (strcmp(cwd, list_get(files, i)) == 0) {
            found = 1;
            break;
        }
    }
    assert(found);
    list_free_deep(files, free);

    // TEST RESOLVE
    assert_resolve("/", "/");
    assert_resolve("/", "///");
    assert_resolve("/abc", "/abc/");
    assert_resolve("/abc", "//abc//");
    assert_resolve_rel("test/abc", "./test/abc/");
    assert_resolve_rel("test/abc", "./test//.//././//abc/");
    assert_resolve_rel("test/abc", "./test//abc//");
    assert_resolve_rel("one/two/three", "one/./two/././three");
    assert_resolve_rel("one", "one");
    assert_resolve("/.abc", "/.abc");
    assert_resolve_rel(".abc", ".//.abc");
    assert_resolve("/abc", "/abc/.");
    assert_resolve("/abc", "/abc/././.");
    assert_resolve("/three", "/one/../two/../three");
    assert_resolve("/three", "/one/two/../../three");
    assert_resolve("/three", "/one/two/../../../three");
    assert_resolve("/one/three", "/one/two/../three");
    assert_resolve("/one/two/.../three", "/one/two/.../three");
    assert_resolve("/one/two/..../three", "/one/two/..../three");
    assert_resolve("/one/two/..test/three", "/one/two/..test/three");
    assert_resolve(cwd, NULL);

    char* cwd_dup = strdup(cwd);
    assert(cwd_dup);

    char* parent_dir = dirname(cwd_dup);

    assert_resolve(parent_dir, "..");
    assert_resolve(cwd, ".");
    assert_resolve(cwd, "");

    char* result = NULL;

    result = fs_resolve_cwd(NULL, "abc/def");
    assert(strcmp("abc/def", result) == 0); free(result);

    result = fs_resolve_cwd(NULL, NULL);
    assert(strcmp(".", result) == 0); free(result);

    result = fs_resolve_cwd("/", "abc/def");
    assert(strcmp("/abc/def", result) == 0); free(result);

    result = fs_resolve_cwd("/home", "abc/def");
    assert(strcmp("/home/abc/def", result) == 0); free(result);

    result = fs_resolve_cwd("home", "abc/def");
    assert(strcmp("home/abc/def", result) == 0); free(result);

    result = fs_resolve_cwd("/home/", "abc/def");
    assert(strcmp("/home/abc/def", result) == 0); free(result);

    free(cwd);
    free(cwd_dup);

    // TEST JOIN PATHS
    assert_join("one/two", "one", "two");
    assert_join("one/two/three", "one/two", "three");
    assert_join("one/two/three", "one/two/", "three");
    assert_join("one/two/three", "one/two/", "/three");
    assert_join("one/two", "one/two/", "/");
    assert_join("one", "one/two/", "..");
    assert_join(".", "one/two/", "../..");
    assert_join("..", "one/two/", "../../..");
    assert_join("..", "", "..");
    assert_join("one", "", "one");
    assert_join("../../test", "one", "../../../test");
    assert_join("../..", "one", "../../../test/..");
    assert_join("one/three", "one/two/..", "three");
    assert_join(".", "one/..", "");
    assert_join("three", ".", "three");
    assert_join("three", "./", "three");
    assert_join("three", "one/..", "three");
    assert_join("a", NULL, "a");
    assert_join("a", "a", NULL);
    assert_join(".", NULL, NULL);

    return 1;
}
