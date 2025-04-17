#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "../main/list.h"

static void* map_fn(void* it) {
    char* old = (char*)it;
    char* new = malloc(strlen(old) + 2 * sizeof(char));
    sprintf(new, "Z%s", old);
    return new;
}

static int filter_fn(void* it) {
    return strlen((char*)it) == 3;
}

static int cmp_fn(const void* a, const void* b) {
    const char* aa = *(const char**)a;
    const char* bb = *(const char**)b;
    int result = -strcmp(aa, bb);
    return result;
}

int list_test(int sz) {
    List* l = list_new(sz);

    // TEST PUSH & POP
    list_push(l, "abc");
    list_push(l, "def");
    list_push(l, "ghi");

    assert(l->size == 3);
    assert(strcmp("abc", list_get(l, 0)) == 0);
    assert(strcmp("def", list_get(l, 1)) == 0);
    assert(strcmp("ghi", list_get(l, 2)) == 0);

    assert(strcmp("ghi", list_pop(l)) == 0);
    assert(l->size == 2);

    list_push(l, "jklm");

    assert(l->size == 3);
    assert(strcmp("jklm", list_get(l, 2)) == 0);

    // TEST FILTERING
    List* filtered = list_filter(l, filter_fn);
    assert(filtered->size == 2);
    assert(strcmp("abc", list_get(filtered, 0)) == 0);
    assert(strcmp("def", list_get(filtered, 1)) == 0);
    list_free(filtered);

    // TEST SORTING
    List* sorted = list_sort(l, cmp_fn);
    assert(sorted->size == 3);
    assert(strcmp("jklm", list_get(sorted, 0)) == 0);
    assert(strcmp("def", list_get(sorted, 1)) == 0);
    assert(strcmp("abc", list_get(sorted, 2)) == 0);
    list_free(sorted);

    // TEST MAPPING
    List* mapped = list_map(l, map_fn);
    assert(mapped->size == 3);
    assert(strcmp("Zabc", list_get(mapped, 0)) == 0);
    assert(strcmp("Zdef", list_get(mapped, 1)) == 0);
    assert(strcmp("Zjklm", list_get(mapped, 2)) == 0);
    list_free_deep(mapped, free);

    // TEST ORIGINAL LIST NOT MUTATED
    assert(l->size == 3);
    assert(strcmp("abc", list_get(l, 0)) == 0);
    assert(strcmp("def", list_get(l, 1)) == 0);
    assert(strcmp("jklm", list_get(l, 2)) == 0);

    // TEST SHIFT & UNSHIFT
    list_unshift(l, "xyz");
    list_unshift(l, "123");
    list_unshift(l, "456");

    assert(l->size == 6);
    assert(strcmp("456", list_get(l, 0)) == 0);
    assert(strcmp("123", list_get(l, 1)) == 0);
    assert(strcmp("xyz", list_get(l, 2)) == 0);
    assert(strcmp("abc", list_get(l, 3)) == 0);

    assert(strcmp("456", list_shift(l)) == 0);
    assert(strcmp("123", list_shift(l)) == 0);
    assert(l->size == 4);

    assert(strcmp("xyz", list_get(l, 0)) == 0);

    assert(strcmp("xyz", list_shift(l)) == 0);
    assert(strcmp("abc", list_shift(l)) == 0);

    assert(l->size == 2);
    assert(strcmp("def", list_get(l, 0)) == 0);
    assert(strcmp("jklm", list_get(l, 1)) == 0);

    // CLEANUP
    list_free(l);

    return 0;
}
