#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "list.h"
#include "hash.h"

// Max load factor for the hash before automatically expanding capacity
#define HASH_LOAD_FACTOR 0.75

struct Hash {
    HashCodeFn hc_fn;   // Function used to determine hash codes for keys
    HashCmpFn cmp_fn;   // Functoin used to compare keys fore equality
    HashEntry** items;  // Array of hash buckets
    size_t capacity;    // Maximum capacity of the hash
    size_t size;        // Current number of entries in the hash
};

struct HashEntry {
    size_t hc;        // Hash code for the item
    HashEntry* next;  // Next item in the same hash bucket
    void* key;        // Key for the hash entry
    void* value;      // Value for the hash entry
};

Hash* hash_new(size_t capacity, HashCodeFn hc_fn, HashCmpFn cmp_fn) {
    Hash* h = NULL;
    HashEntry** items = NULL;

    capacity = capacity < 1 ? 1 : capacity;

    h = malloc(sizeof(Hash));
    items = malloc(capacity * sizeof(HashEntry*));

    if (!h || !items) goto error;

    for (size_t i = 0; i < capacity; i++) items[i] = NULL;

    h->hc_fn = hc_fn;
    h->cmp_fn = cmp_fn;
    h->capacity = capacity;
    h->size = 0;
    h->items = items;
    return h;

error:
    free(h);
    free(items);
    errno = ENOMEM;
    return NULL;
}

HashEntry* hash_put_entry(Hash* h, HashEntry* e) {
    size_t capacity = h->capacity;
    size_t idx = e->hc % capacity;
    void* key = e->key;

    HashEntry** head = &h->items[idx];
    for (HashEntry* entry = *head; entry != NULL; head = &entry->next, entry = *head) {
        if (h->cmp_fn(entry->key, key) == 0) {
            *head = e;
            e->next = entry->next;
            return entry;
        }
    }
    *head = e;
    e->next = NULL;
    h->size++;
    return NULL;
}

HashPutResult hash_put(Hash* h, void* key, void* value) {
    HashPutResult result = {
        .old_entry = NULL,
        .new_entry = NULL,
        .status = HASH_STATUS_ENOMEM,
    };

    result.new_entry = malloc(sizeof(HashEntry));
    if (!result.new_entry) goto error;
    result.new_entry->key = key;
    result.new_entry->value = value;
    result.new_entry->hc = h->hc_fn(key);
    result.old_entry = hash_put_entry(h, result.new_entry);

    #ifdef HASH_LOAD_FACTOR
    float load_factor = (float)h->size / (float)h->capacity;
    if (load_factor > HASH_LOAD_FACTOR) {
        result.status = hash_resize(h, h->capacity * 2);
        if (result.status != HASH_STATUS_OK) goto error;
    }
    #endif

    result.status = HASH_STATUS_OK;

    return result;

error:
    free(result.new_entry);
    result.new_entry = NULL;
    errno = ENOMEM;
    return result;
}

HashEntry* hash_remove(Hash* h, void* key) {
    size_t hc = h->hc_fn(key);
    size_t capacity = h->capacity;
    size_t idx = hc % capacity;

    HashEntry** head = &h->items[idx];
    for (HashEntry* entry = *head; entry != NULL; head = &entry->next, entry = *head) {
        if (h->cmp_fn(entry->key, key) == 0) {
            *head = entry->next;
            h->size--;
            return entry;
        }
    }
    return NULL;
}

HashEntry* hash_get_entry(Hash* h, void* key) {
    size_t hc = h->hc_fn(key);
    size_t idx = hc % h->capacity;

    for (HashEntry* entry = h->items[idx]; entry != NULL; entry = entry->next) {
        if (h->cmp_fn(entry->key, key) == 0) {
            return entry;
        }
    }
    return NULL;
}

void* hash_get(Hash* h, void* key) {
    HashEntry* e = hash_get_entry(h, key);
    return e == NULL ? NULL : e->value;
}

int hash_contains_key(Hash* h, void* key) {
    return hash_get_entry(h, key) != NULL;
}

List* hash_entries(Hash* h) {
    List* entries = list_new(h->size);
    if (!entries) goto error;

    for (size_t i = 0; i < h->capacity; i++) {
        for (HashEntry* entry = h->items[i]; entry != NULL; entry = entry->next) {
            if (list_push(entries, entry) != LIST_STATUS_OK) goto error;
        }
    }

    return entries;

error:
    list_free(entries);
    errno = ENOMEM;
    return NULL;
}

inline void* hash_entry_key(HashEntry* e) {
    return e ? e->key : NULL;
}

inline void* hash_entry_value(HashEntry* e) {
    return e ? e->value : NULL;
}

static inline void* hash_entry_get_key(void* item) {
    return hash_entry_key((HashEntry*)item);
}

static inline void* hash_entry_get_value(void* item) {
    return hash_entry_value((HashEntry*)item);
}

List* hash_keys(Hash* h) {
    List* entries = hash_entries(h);
    if (!entries) return NULL;
    List* keys = list_map(entries, hash_entry_get_key);
    list_free(entries);
    return keys;
}

List* hash_values(Hash* h) {
    List* entries = hash_entries(h);
    if (!entries) return NULL;
    List* values = list_map(entries, hash_entry_get_value);
    list_free(entries);
    return values;
}

HashStatusCode hash_resize(Hash* h, size_t capacity) {
    HashEntry** new_items = NULL;

    size_t old_capacity = h->capacity;
    HashEntry** old_items = h->items;

    new_items = malloc(capacity * sizeof(HashEntry*));
    if (!new_items) goto error;

    for (size_t i = 0; i < capacity; i++) new_items[i] = NULL;

    h->size = 0;
    h->items = new_items;
    h->capacity = capacity;

    for (size_t i = 0; i < old_capacity; i++) {
        for (HashEntry* entry = old_items[i]; entry; ) {
            HashEntry* tmp_next = entry->next;
            entry->next = NULL;
            hash_put_entry(h, entry);
            entry = tmp_next;
        }
    }

    free(old_items);
    return HASH_STATUS_OK;

error:
    free(new_items);
    errno = ENOMEM;
    return HASH_STATUS_ENOMEM;
}

inline size_t hash_size(Hash* h) {
    return h->size;
}

void hash_entry_free(HashEntry* h) {
    free(h);
}

void hash_free(Hash* h) {
    hash_free_deep(h, hash_entry_free);
}

void hash_free_deep(Hash* h, HashEntryFreeFn fn) {
    if (h != NULL) {
        HashEntry** items = h->items;
        for (size_t i = 0; i < h->capacity; i++) {
            for (HashEntry* entry = items[i]; entry != NULL; ) {
                HashEntry* next = entry->next;
                fn(entry);
                entry = next;
            }
        }
        free(items);
    }
    free(h);
}

size_t hash_code_str(void* val) {
    char* str = (char*)val;
    size_t hc = 0;
    for (size_t i = 0; i < strlen(str); i++) {
        hc *= 7;
        hc += str[i];
    }
    return hc;
}

int hash_cmp_str(void* a, void* b) {
    return strcmp(a, b);
}

Hash* hash_new_str(size_t capacity) {
    return hash_new(capacity, hash_code_str, hash_cmp_str);
}
