#ifndef _LIST_H_
#define _LIST_H_

/**
 * List structure. Use list_new to create one.
 */
typedef struct List List;

/**
 * Status codes for various list operations
 */
typedef enum {
    LIST_STATUS_OK,      ///< Operation succeeded
    LIST_STATUS_ENOMEM,  ///< Failed due to allocation error
} ListStatusCode;

/**
 * Callback function for the list_filter operation. This callback is executed
 * once for each item in the list and is expected to return truthy if the
 * item should be added to the newly created list, or zero if not.
 * @param item  each item of the list
 * @return      truthy if the item should be added to the new list
 */
typedef int (*ListFilterFn)(void* item);

/**
 * Callback function for the list_map operation. This callback is executed
 * once for each item in the list and is expected to return a value to be
 * added into the newly created list.
 * @param item  each item of the list
 * @return      the new list item
 */
typedef void* (*ListMapFn)(void* item);

/**
 * Callback function for the list_each operation. This callback is executed
 * once for each item in the list and is not expected to produce a result.
 * @param item  each item of the list
 */
typedef void (*ListEachFn)(void* item);

/**
 * Callback function for the list_sort operation. Compares two items in the
 * list to determine what order they should be sorted in.
 * @param a  first item to compare
 * @param b  second item to compare
 * @return   negative if a should be sorted before b, zero if a should be
 *           sorted equal to b, and postiive if a should be sorted after b
 */
typedef int (*ListCmpFn)(const void* a, const void* b);

/**
 * Callback function to free an item of the list. Used by the list_free_deep
 * operation which frees the list itself and all items within the list.
 * @param item  to be freed
 */
typedef void (*ListItemFreeFn)(void* item);

/**
 * Used with the list_filter_data operation. Same as ListFilterFn, but accepts
 * an additional data parameter for data to be passed into the callback.
 * @param item  each item of the list
 * @param data  opaque context data passed into each callback
 * @return      truthy if the item should be added to the new list
 */
typedef int (*ListFilterDataFn)(void* item, void* data);

/**
 * Used with the list_map_data operation. Same as ListMapFn, but accepts
 * an additional data parameter for data to be passed into the callback.
 * @param item  each item of the list
 * @param data  opaque context data passed into each callback
 * @return      the new list item
 */
typedef void* (*ListMapDataFn)(void* item, void* data);

/**
 * Used with the list_each_data operation. Same as ListEachFn, but accepts
 * an additional data parameter for data to be passed into the callback.
 * @param item  each item of the list
 * @param data  opaque context data passed into each callback
 */
typedef void (*ListEachDataFn)(void* item, void* data);

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
 * Implements a ring buffer array for efficient push/pop operations with
 * amortized expansion. Free it with list_free when you're done.
 * @param capacity  initial list capacity
 * @return          pointer to the new List, or NULL in case of failure
 */
List* list_new(size_t capacity);

/**
 * Retrieve the number of items in the list.
 * @param l  to check the size of
 * @return   current size of the list
 */
size_t list_size(List* l);

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
List* list_sort(List* l, ListCmpFn);

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
