#ifndef _LIST_H_
#define _LIST_H_

/**
 * List structure. Use list_new to create one.
 */
typedef struct {
    void** items;
    size_t start_idx;
    size_t size;
    size_t capacity;
} List;

/**
 * Status codes for various list operations
 */
typedef enum {
    LIST_STATUS_OK,
    LIST_STATUS_ENOMEM,
} ListStatusCode;

// callback functions for map/filter/each/sort/free operations
typedef int (*ListFilterFn)(void*);
typedef void* (*ListMapFn)(void*);
typedef void (*ListEachFn)(void*);
typedef int (*ListCompFn)(const void*, const void*);
typedef void (*ListItemFreeFn)();

// equivalent callbacks that accept an additional data param
typedef int (*ListFilterDataFn)(void*, void*);
typedef void* (*ListMapDataFn)(void*, void*);
typedef void (*ListEachDataFn)(void*, void*);
typedef void (*ListItemFreeDataFn)(void*);

/**
 * Resize the internal capacity of the list.
 * If the new capacity is smaller than the size, items at the end of
 * the list will be lost.
 * @param l         list to resize
 * @param capacity  new capacity for the list
 * @return          list status
 */
ListStatusCode list_resize(List* l, size_t capacity);

/**
 * Allocate a new list.
 * Implements a ring buffer array for efficient push/pop operations
 * with amortized time expansion.
 * @param capacity  initial list capacity
 * @return          pointer to the new List
 */
List* list_new(size_t capacity);

/**
 * Prepend an item to the beginning of the list.
 * May expand the size of the list if the capacity is less than the
 * new size. Check the return status to make sure it worked.
 * Generally constant time. Resizing is O(n).
 * @param l     list to prepend to
 * @param item  to prepend to the list
 * @return      list status
 */
ListStatusCode list_unshift(List* l, void* item);

/**
 * Prepend another list to the beginning. May expand capacity.
 * Check the return status to make sure it worked.
 * @param l     list to prepend to
 * @param that  other list
 * @return      list status
 */
ListStatusCode list_unshift_all(List* l, List* that);

/**
 * Append an item to the end of the list.
 * May expand the size of the list if the capacity is less than the
 * new size. Check the return status to make sure it worked.
 * Generally constant time. Resizing is O(n).
 * @param l     list to append to
 * @param item  to append to the list
 * @return      list status
 */
ListStatusCode list_push(List* l, void* item);

/**
 * Append another list to the end. May expand capacity.
 * Check the return status to make sure it worked.
 * @param l     list to append to
 * @param that  other list
 * @return      list status
 */
ListStatusCode list_push_all(List* l, List* that);

/**
 * Remove and return the first item of the list.
 * Constant time.
 * @param l     list to pop from
 * @return      former first item of the list
 */
void* list_pop(List* l);

/**
 * Remove and return the last item of the list.
 * Constant time.
 * @param l     list to pop from
 * @return      former last item of the list
 */
void* list_shift(List* l);

/**
 * Get the item at a specific index. Constant time.
 * @param l  list to get from
 * @param i  index to get
 * @return   item at index i
 */
void* list_get(List* l, size_t i);

/**
 * Set the item at a specific index. Constant time.
 * @param l     list to set item
 * @param i     index to set
 * @param item  new item to assign at index i
 * @return      replaced item at index i
 */
void* list_set(List* l, size_t i, void* item);

/**
 * Free the list. Does not free individual items.
 * @param l  list to free
 */
void list_free(List* l);

/**
 * Free the list and all items.
 * @param l  list to free
 * @param f  function to use to free each item
 */
void list_free_deep(List* l, ListItemFreeFn);

/**
 * Create a copy of the list retaining only items that satisfy the
 * provided predicate condition. Original list is not altered.
 * Can fail when out of memory. Check that the result is not NULL.
 * O(n) time to create a copy, plus your filter funcion on each item.
 * @param l  list to filter
 * @param f  predicate function
 * @         filtered copy of the list
 */
List* list_filter(List* l, ListFilterFn);

/**
 * Create a copy of the list by transforming each item.
 * Original list is not altered. Can fail when out of memory.
 * Check that the result is not NULL.
 * O(n) time to create a copy, plus your map funcion on each item.
 * @param l  list to map
 * @param f  transformer function
 * @         transformed copy of the list
 */
List* list_map(List* l, ListMapFn);

/**
 * Create a sorted copy of the list based on the provided sort function.
 * Original list is not altered. Can fail when out of memory.
 * Check that the result is not NULL.
 * O(n) time to create a copy, plus qsort (O(n log(n)))
 * @param l  list to map
 * @param f  transformer function
 * @         transformed copy of the list
 */
List* list_sort(List* l, ListCompFn);

/**
 * Iterate the list and run a callback function for each item.
 * O(n) time to iterate, plus your callback function.
 * @param l  list to map
 * @param f  transformer function
 * @         transformed copy of the list
 */
void list_each(List* l, ListEachFn);

/**
 * Create a copy of the list retaining only items that satisfy the
 * provided predicate condition. Original list is not altered.
 * Can fail when out of memory. Check that the result is not NULL.
 * O(n) time to create a copy, plus your filter funcion on each item.
 * @param l     list to filter
 * @param f     predicate function
 * @param data  context data to pass into the predicate
 * @            filtered copy of the list
 */
List* list_filter_data(List* l, ListFilterDataFn, void* data);

/**
 * Create a copy of the list by transforming each item.
 * Original list is not altered. Can fail when out of memory.
 * Check that the result is not NULL.
 * O(n) time to create a copy, plus your map funcion on each item.
 * @param l     list to map
 * @param f     transformer function
 * @param data  context data to pass into the transformer
 * @            transformed copy of the list
 */
List* list_map_data(List* l, ListMapDataFn, void* data);

/**
 * Iterate the list and run a callback function for each item.
 * O(n) time to iterate, plus your callback function.
 * @param l     list to map
 * @param f     transformer function
 * @param data  context data to pass into the transformer
 * @            transformed copy of the list
 */
void list_each_data(List* l, ListEachDataFn, void* data);

#endif
