#ifndef _HASH_H_
#define _HASH_H_

#include "list.h"

/**
 * Create a hash code for a key value. Must obey the hashcode/equals contract:
 *  - Must consistently return the same hash code for the same key
 *  - If two objects are equal, they must return the same hash code
 *  - Unequal keys should return different hash codes, but are not required to
 * @param key  to create a hash code for
 * @return     the hash code
 */
typedef size_t (*HashCodeFn)(void* key);

/**
 * Compare two hash keys for equality. Must be consistent with the HashCodeFn
 * used when constructing the hash. Must obey the hashcode/equals contract:
 *  - Reflexive: a must = a
 *  - Symmetric: a = b if and only if b = a
 *  - Transitive: if a = b and b = c, then a must = c
 *  - Consistent: the result of a = b must always be the same
 * @param a  first key to compare
 * @param b  second key to compare
 * @return   zero if a and b are equal, non-zero otherwise
 */
typedef int (*HashCmpFn)(void* a, void* b);

/**
 * Status of hash operations.
 */
typedef enum {
    HASH_STATUS_OK,      ///< Operation successful
    HASH_STATUS_ENOMEM,  ///< Failed due to memory allocation error
} HashStatusCode;

/**
 * Represents an key/value pair entry in the hash.
 */
typedef struct HashEntry HashEntry;

/**
 * Result of a hash_put operation. May replace an existing item in the hash if
 * the key is the same. You should free the old_entry with hash_entry_free in
 * case this operation replaced an item.
 */
typedef struct {
    HashEntry* old_entry;  ///< The prior entry for this key, or NULL if empty
    HashEntry* new_entry;  ///< The new hash entry created
    HashStatusCode status; ///< Status of the hash_put operation
} HashPutResult;

/**
 * A hash table. Use hash_new to create one.
 */
typedef struct Hash Hash;

/**
 * Callback function to free an item of the list. Used by the list_free_deep
 * operation which frees the list itself and all items within the list.
 * @param item  to be freed
 */
typedef void (*HashEntryFreeFn)(HashEntry* entry);

/**
 * Allocate a new hash.
 * Implements a hash table for efficiently storing key/value pairs.
 * Amortized expansion when the capacity reaches a defined load capacity.
 * The put/get/remove operations are constant time in best cases but degrade
 * into linear time when collissions occur. Collissions are stored in a linked
 * list and searched linearly. Free it with hash_free when you're done.
 * @param capacity  initial hash capacity
 * @param hc_fn     hash code function to use when inserting into the hash
 * @param cmp_fn    equality function to use when hash collisions occur
 * @return          pointer to the new Hash, or NULL in case of failure
 */
Hash* hash_new(size_t capacity, HashCodeFn hc_fn, HashCmpFn cmp_fn);

/**
 * Retrieve the number of items in the hash.
 * @param h  to check the size of
 * @return   current size of the list
 */
size_t hash_size(Hash* h);

/**
 * Insert or replace the value for a specific key. Since this operation may
 * replace an existing HashEntry, make sure to free the old_entry that is
 * returned by this operation with hash_free_entry.
 * @param h      hash to operate on
 * @param key    to update in the map
 * @param value  to update in the map
 * @return       struct containing the result of the operation
 */
HashPutResult hash_put(Hash* h, void* key, void* value);

/**
 * Remove and return the entry for the specific key. This operation does not
 * free the hash entry, so use hash_free_entry when you are done.
 * @param h    hash to operate on
 * @param key  key for the entry to remove
 * @return     the removed hash entry, or NULL if there was none
 */
HashEntry* hash_remove(Hash* h, void* key);

/**
 * Retrieve the value for the specified hash key.
 * @param h    hash to operate on
 * @param key  to retrieve
 * @return     value for the key, or NULL if the key is not present
 */
void* hash_get(Hash* h, void* key);

/**
 * Check if the hash contains a specific key.
 * @param h    hash to operate on
 * @param key  to check
 * @return     truthy if and only if the key is present
 */
int hash_contains_key(Hash* h, void* key);

/**
 * Resizes the internal capacity of the hash.
 * @param h         hash to resize
 * @param capacity  new capacity for the hash
 * @return          hash status
 */
HashStatusCode hash_resize(Hash* h, size_t capacity);

/**
 * Get the key associated with a HashEntry.
 * @param e  entry to operate on
 * @return   the associated key
 */
void* hash_entry_key(HashEntry* e);

/**
 * Get the value associated with a HashEntry.
 * @param e  entry to operate on
 * @return   the associated value
 */
void* hash_entry_value(HashEntry* e);

/**
 * Retrieve a list of all HashEntry entries present in the hash.
 * This allocats a new list, free it with list_free when you're done.
 * @param h  hash to operate on
 * @return   list of HashEntry, or NULL if an error occurred
 */
List* hash_entries(Hash* h);

/**
 * Retrieve a list of all keys in the hash.
 * This allocats a new list, free it with list_free when you're done.
 * @param h  hash to operate on
 * @return   list of keys, or NULL if an error occurred
 */
List* hash_keys(Hash* h);

/**
 * Retrieve a list of all values in the hash.
 * This allocats a new list, free it with list_free when you're done.
 * @param h  hash to operate on
 * @return   list of values, or NULL if an error occurred
 */
List* hash_values(Hash* h);

/**
 * Create a new hash for string-based keys.
 * @param capacity  initial hash capacity
 * @return          pointer to the new Hash
 */
Hash* hash_new_str(size_t capacity);

/**
 * HashCodeFn for string values.
 * @param val  string to calculate a hash code for
 * @return     hash code
 */
size_t hash_code_str(void* val);

/**
 * HashCmpFn for string values.
 * @param a  first string to compare
 * @param b  second string to compare
 * @return   zero if the strings are equal
 */
int hash_cmp_str(void* a, void* b);

/**
 * Free the Hash and memory allocated to store HashEntry structs. This does not
 * automatically free any keys or values contained in the hash.
 * @param h  hash to free
 */
void hash_free(Hash* h);

/**
 * Free an individual HashEntry. Use it with hash_put and hash_remove to free
 * any replaced/removed entries from those functions. This does not free any
 * keys or values contiained in the hash.
 * @param h  hash entry to free
 */
void hash_entry_free(HashEntry* h);

/**
 * Free the hash and all keys/values using the specified callback. The provided
 * callback should also use hash_entry_free to free the entry itself when done.
 * @param h        hash to free
 * @param free_fn  function to apply to free each HashEntry
 */
void hash_free_deep(Hash* h, HashEntryFreeFn free_fn);

#endif
