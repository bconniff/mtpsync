#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "../main/hash.h"
#include "../main/list.h"

int hash_test(int sz) {
    Hash* h = hash_new_str(sz);
    assert(h);
    assert(hash_size(h) == 0);

    // Remove on empty map does nothing
    assert(hash_remove(h, "NOPE") == NULL);

    assert(hash_size(h) == 0);

    // TEST PUT & GET
    hash_entry_free(hash_put(h, "help", "test").old_entry);
    hash_entry_free(hash_put(h, "hello", "world").old_entry);
    hash_entry_free(hash_put(h, "last", "value").old_entry);

    assert(hash_size(h) == 3);

    assert(strcmp("test", (char*)hash_get(h, "help")) == 0);
    assert(strcmp("world", (char*)hash_get(h, "hello")) == 0);
    assert(strcmp("value", (char*)hash_get(h, "last")) == 0);
    assert((char*)hash_get(h, "NOPE") == NULL);

    // TEST PUT EXISTING KEY
    HashPutResult result = hash_put(h, "help", "REPLACE");

    assert(strcmp("test", hash_entry_value(result.old_entry)) == 0);

    hash_entry_free(result.old_entry);

    assert(hash_size(h) == 3);

    assert(strcmp("REPLACE", (char*)hash_get(h, "help")) == 0);
    assert(strcmp("world", (char*)hash_get(h, "hello")) == 0);
    assert(strcmp("value", (char*)hash_get(h, "last")) == 0);

    // TEST REMOVE
    assert(hash_size(h) == 3);
    hash_entry_free(hash_remove(h, "help"));
    hash_entry_free(hash_remove(h, "hello"));
    hash_entry_free(hash_remove(h, "NOPE"));
    assert(hash_size(h) == 1);

    // TEST ENTRIES
    List* l = hash_entries(h);

    assert(list_size(l) == 1);

    HashEntry* e = list_get(l, 0);
    assert(strcmp("last", hash_entry_key(e)) == 0);
    assert(strcmp("value", hash_entry_value(e)) == 0);

    // CLEANUP
    list_free(l);
    hash_free(h);

    return 0;
}
