#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "../main/str.h"

static void assert_join(char* expected, char* joined) {
    assert(strcmp(expected, joined) == 0);
    free(joined);
}

int str_test() {
    // TEST STARTS WITH
    assert(str_starts_with("abc def ghi", "abc"));
    assert(!str_starts_with("abc def ghi", "abcd"));
    assert(!str_starts_with("abc", "abcde"));
    assert(str_starts_with("abc", "abc"));

    // TEST ENDS WITH
    assert(str_ends_with("abc def ghi", "ghi"));
    assert(!str_ends_with("abc def ghi", "fghi"));
    assert(!str_ends_with("abc", "abcde"));
    assert(str_ends_with("abc", "abc"));

    // TEST JOIN STRINGS
    assert_join("abcdefghi", str_join(3, "abc", "def", "ghi"));
    assert_join("abcdef", str_join(2, "abc", "def", "ghi"));

    size_t sz = STR_BUF_SIZE * 2;

    char* long_str_test1 = malloc((sz+1) * sizeof(char));
    assert(long_str_test1);
    char* long_str_test2 = malloc((sz+1) * sizeof(char));
    assert(long_str_test2);

    for (size_t i = 0; i < sz; i++) {
        long_str_test1[i] = 'a';
        long_str_test2[i] = 'b';
    }

    long_str_test1[sz] = 0;
    long_str_test2[sz] = 0;

    char* result = str_join(2, long_str_test1, long_str_test2);

    assert('a' == result[0]);
    assert('b' == result[sz]);
    assert(0 == result[sz*2]);
    assert(strlen(result) == (sz * 2));

    free(long_str_test1);
    free(long_str_test2);
    free(result);

    return 0;
}
