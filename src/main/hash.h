#ifndef _HASH_H_
#define _HASH_H_

#include "list.h"

#define HASH_LOAD_FACTOR 0.75

typedef size_t (*HashCodeFn)(void*);
typedef int (*HashCmpFn)(void*, void*);

typedef enum {
    HASH_STATUS_OK,
    HASH_STATUS_ENOMEM,
} HashStatusCode;

typedef struct HashEntryStruct {
    size_t hc;
    struct HashEntryStruct* next;
    void* key;
    void* value;
} HashEntry;

typedef struct {
    HashEntry* old_entry;
    HashEntry* new_entry;
    HashStatusCode status;
} HashPutResult;

typedef struct {
    HashCodeFn hc_fn;
    HashCmpFn cmp_fn;
    HashEntry** items;
    size_t capacity;
    size_t size;
} Hash;

typedef void (*HashEntryFreeFn)(HashEntry*);

// basic hash functions
Hash* hash_new(size_t capacity, HashCodeFn hc_fn, HashCmpFn cmp_fn);
HashPutResult hash_put(Hash* h, void* key, void* item);
HashEntry* hash_remove(Hash* h, void* key);
void* hash_get(Hash* h, void* key);
int hash_contains_key(Hash* h, void* key);
HashStatusCode hash_resize(Hash* h, size_t capacity);
List* hash_entries(Hash* h);
List* hash_keys(Hash* h);
List* hash_values(Hash* h);

// support for hashes with string keys
Hash* hash_new_str(size_t capacity);
size_t hash_code_str(void* val);
int hash_cmp_str(void* a, void* b);

// free
void hash_free(Hash* h);
void hash_entry_free(HashEntry* h);
void hash_free_deep(Hash* h, HashEntryFreeFn free_fn);

#endif
