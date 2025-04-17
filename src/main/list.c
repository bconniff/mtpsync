#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "list.h"

static inline void* list_map_fn_without_data(void* item, void* data) {
    ListMapFn fn = (ListMapFn)data;
    return fn(item);
}

static inline void list_each_fn_without_data(void* item, void* data) {
    ListEachFn fn = (ListEachFn)data;
    fn(item);
}

static inline int list_filter_fn_without_data(void* item, void* data) {
    ListFilterFn fn = (ListFilterFn)data;
    return fn(item);
}
static inline int list_real_index(List* l, size_t i) {
    return (l->start_idx + i) % l->capacity;
}

static inline int list_valid_index(List* l, size_t i) {
    return i >= 0 && i < l->size;
}

List* list_new(size_t capacity) {
    List* l = NULL;
    void** items = NULL;

    capacity = capacity < 1 ? 1 : capacity;

    l = malloc(sizeof(List));
    items = malloc(capacity * sizeof(void*));

    if (!l || !items) goto error;

    l->capacity = capacity;
    l->start_idx = 0;
    l->size = 0;
    l->items = items;
    return l;

error:
    free(l);
    free(items);
    errno = ENOMEM;
    return NULL;
}

ListStatusCode list_resize(List* l, size_t capacity) {
    void** new_items = malloc(capacity * sizeof(void*));
    if (!new_items) goto error;

    size_t new_size = l->size < capacity ? l->size : capacity;
    for (size_t i = 0; i < new_size; i++) {
        new_items[i] = list_get(l, i);
    }

    free(l->items);
    l->start_idx = 0;
    l->items = new_items;
    l->capacity = capacity;
    l->size = new_size;

    return LIST_STATUS_OK;

error:
    free(new_items);
    errno = ENOMEM;
    return LIST_STATUS_ENOMEM;
}

ListStatusCode list_push(List* l, void* item) {
    size_t size = l->size;
    size_t capacity = l->capacity;

    if (size == capacity) {
        if (list_resize(l, capacity * 2) != LIST_STATUS_OK) goto error;
    }

    l->size = size + 1;
    list_set(l, size, item);
    return LIST_STATUS_OK;

error:
    errno = ENOMEM;
    return LIST_STATUS_ENOMEM;
}

ListStatusCode list_unshift(List* l, void* item) {
    size_t size = l->size;
    size_t capacity = l->capacity;

    if (size == capacity) {
        capacity *= 2;
        if (list_resize(l, capacity) != LIST_STATUS_OK) goto error;
    }

    size_t start_idx = l->start_idx;
    start_idx = (start_idx == 0 ? capacity : start_idx) - 1;

    l->start_idx = start_idx;
    l->size++;
    list_set(l, 0, item);
    return LIST_STATUS_OK;

error:
    errno = ENOMEM;
    return LIST_STATUS_ENOMEM;
}

ListStatusCode list_push_all(List* l, List* that) {
    for (size_t i = 0; i < that->size; i++) {
        if (list_push(l, list_get(that, i)) != LIST_STATUS_OK) goto error;
    }
    return LIST_STATUS_OK;

error:
    errno = ENOMEM;
    return LIST_STATUS_ENOMEM;
}

ListStatusCode list_unshift_all(List* l, List* that) {
    for (size_t i = 0; i < that->size; i++) {
        if (list_unshift(l, list_get(that, i)) != LIST_STATUS_OK) goto error;
    }
    return LIST_STATUS_OK;

error:
    errno = ENOMEM;
    return LIST_STATUS_ENOMEM;
}

void* list_pop(List* l) {
    size_t size = l->size;
    if (size == 0) {
        return NULL;
    }

    void* item = list_get(l, --size);
    l->size = size;
    return item;
}

void* list_shift(List* l) {
    size_t size = l->size;
    if (size == 0) {
        return NULL;
    }

    void* item = list_get(l, 0);
    l->start_idx = (l->start_idx + 1) % l->capacity;
    l->size--;
    return item;
}

inline void* list_get(List* l, size_t i) {
    return list_valid_index(l, i) ? l->items[list_real_index(l, i)] : NULL;
}

inline void* list_set(List* l, size_t i, void* it) {
    void* tmp = NULL;
    if (list_valid_index(l, i)) {
        size_t real_idx = list_real_index(l, i);
        tmp = l->items[real_idx];
        l->items[real_idx] = it;
    }
    return tmp;
}

void list_free(List* l) {
    if (l != NULL) {
        free(l->items);
    }
    free(l);
}

List* list_filter_data(List* l, ListFilterDataFn f, void* data) {
    List* result = list_new(l->capacity);

    if (!result) goto error;

    for (size_t i = 0; i < l->size; i++) {
        void* item = list_get(l, i);
        if (f(item, data)) {
            ListStatusCode code = list_push(result, item);
            if (code != LIST_STATUS_OK) goto error;
        }
    }
    return result;

error:
    list_free(result);
    errno = ENOMEM;
    return NULL;
}

List* list_map_data(List* l, ListMapDataFn f, void* data) {
    List* result = list_new(l->capacity);

    if (!result) goto error;

    for (size_t i = 0; i < l->size; i++) {
        void* item = list_get(l, i);
        if (list_push(result, f(item, data)) != LIST_STATUS_OK) goto error;
    }
    return result;

error:
    list_free(result);
    errno = ENOMEM;
    return NULL;
}

void list_each_data(List* l, ListEachDataFn f, void* data) {
    for (size_t i = 0; i < l->size; i++) {
        void* item = list_get(l, i);
        f(item, data);
    }
}

List* list_map(List* l, ListMapFn f) {
    return list_map_data(l, list_map_fn_without_data, f);
}

List* list_filter(List* l, ListFilterFn f) {
    return list_filter_data(l, list_filter_fn_without_data, f);
}

void list_each(List* l, ListEachFn f) {
    return list_each_data(l, list_each_fn_without_data, f);
}

List* list_sort(List* l, ListCompFn comp) {
    List* result = list_new(l->capacity);

    if (!result) goto error;

    // this should ensure the new list is contiguous
    list_push_all(result, l);
    qsort(result->items, result->size, sizeof(void*), comp);
    return result;

error:
    list_free(result);
    errno = ENOMEM;
    return NULL;
}

void list_free_deep(List* l, ListItemFreeFn f) {
    if (l != NULL) {
        for (size_t i = 0; i < l->size; i++) {
            f(list_get(l, i));
        }
    }
    list_free(l);
}
