#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>

#include "sync.h"
#include "str.h"
#include "list.h"
#include "hash.h"
#include "fs.h"
#include "mtp.h"

typedef enum {
    SYNC_STATUS_OK,
    SYNC_STATUS_EFAIL,
} SyncStatusCode;

typedef SyncStatusCode (*SyncAncestorFn)(char* target, int is_ancestor, void* data);

static inline int sync_plan_cmp(const void* a, const void* b) {
    const SyncPlan* aa = *(const SyncPlan**)a;
    const SyncPlan* bb = *(const SyncPlan**)b;

    int cmp = 0;

    // first sort by action
    if (!cmp) cmp = aa->action - bb->action;

    // action-specific sorting
    if (!cmp) {
        switch (aa->action) {
            // for deletion, sort longest paths first
            case SYNC_ACTION_RM:
                cmp = str_count_char(bb->target, '/') - str_count_char(aa->target, '/');
                break;

            // for new dirs, sort shortest paths first
            case SYNC_ACTION_MKDIR:
                cmp = str_count_char(aa->target, '/') - str_count_char(bb->target, '/');
                break;

            default:
        }
    }

    // last, sort alphabetically
    if (!cmp) cmp = strcmp(aa->target, bb->target);

    return cmp;
}

static SyncStatusCode put_ancestors(Hash* hash, char* f, SyncAncestorFn fn, void* data) {
    int code = SYNC_STATUS_EFAIL;
    char* target = NULL;

    target = strdup(f);
    if (!target) goto done;

    int is_ancestor = 0;

    do {
        HashPutResult r = hash_put(hash, target, target);
        int is_existing = r.old_entry != NULL;
        free(hash_entry_key(r.old_entry));
        hash_entry_free(r.old_entry);
        if (r.status != HASH_STATUS_OK) goto done;

        if (is_existing) break;

        if (fn && (fn(target, is_ancestor, data) != SYNC_STATUS_OK)) goto done;

        target = fs_dirname(target);
        if (!target) goto done;

        is_ancestor = 1;
    } while (strcmp(target, "/") != 0);

    target = NULL;

    code = SYNC_STATUS_OK;

done:
    free(target);
    return code;
}

void sync_plan_free(SyncPlan* plan) {
    if (plan) {
        free(plan->source);
        free(plan->target);
        free(plan);
    }
}

SyncPlan* sync_plan_new(char* source, char* target, SyncAction action) {
    char* source_dup = NULL;
    char* target_dup = NULL;
    SyncPlan* plan = NULL;

    if (source) {
        source_dup = strdup(source);
        if (!source_dup) goto error;
    }

    if (target) {
        target_dup = strdup(target);
        if (!target_dup) goto error;
    }

    plan = malloc(sizeof(SyncPlan));
    if (!plan) goto error;
    plan->source = source_dup;
    plan->target = target_dup;
    plan->action = action;
    return plan;

error:
    free(source_dup);
    free(target_dup);
    free(plan);
    return NULL;
}

void sync_spec_free(SyncSpec* plan) {
    if (plan) {
        free(plan->source);
        free(plan->target);
        free(plan);
    }
}

SyncSpec* sync_spec_new(char* source, char* target) {
    char* source_dup = NULL;
    char* target_dup = NULL;
    SyncSpec* spec = NULL;

    if (source) {
        source_dup = strdup(source);
        if (!source_dup) goto error;
    }

    if (target) {
        target_dup = strdup(target);
        if (!target_dup) goto error;
    }

    spec = malloc(sizeof(SyncSpec));
    if (!spec) goto error;
    spec->source = source_dup;
    spec->target = target_dup;
    return spec;

error:
    free(source_dup);
    free(target_dup);
    free(spec);
    return NULL;
}

static inline SyncStatusCode ancestor_cb(char* target, int is_ancestor, void* data) {
    SyncPlan* plan = NULL;

    if (is_ancestor) {
        List* plans = data;

        plan = sync_plan_new(NULL, target, SYNC_ACTION_MKDIR);
        if (!plan) goto error;

        if (list_push(plans, plan) != LIST_STATUS_OK) goto error;
    }

    return SYNC_STATUS_OK;

error:
    sync_plan_free(plan);
    return SYNC_STATUS_EFAIL;
}

List* sync_plan(List* specs, List* target_files) {
    Hash* expected_hash = NULL;
    Hash* target_hash = NULL;
    List* target_files_after = NULL;
    List* plans = NULL;
    List* plans_sorted = NULL;
    SyncPlan* plan_tmp = NULL;

    plans = list_new((list_size(target_files) + list_size(specs)) * 2);
    if (!plans) goto error;

    target_hash = hash_new_str(list_size(target_files) * 2);
    if (!target_hash) goto error;

    for (size_t i = 0; i < list_size(target_files); i++) {
        char* f = list_get(target_files, i);

        if (put_ancestors(target_hash, f, NULL, NULL) != SYNC_STATUS_OK) goto error;
    }

    expected_hash = hash_new_str(list_size(specs) * 2);
    if (!expected_hash) goto error;

    for (size_t i = 0; i < list_size(specs); i++) {
        SyncSpec* spec = list_get(specs, i);
        char* f = spec->target;

        if (put_ancestors(expected_hash, f, NULL, NULL) != SYNC_STATUS_OK) goto error;

        if (!hash_get(target_hash, f)) {
            if (put_ancestors(target_hash, f, ancestor_cb, plans) != SYNC_STATUS_OK) goto error;

            plan_tmp = sync_plan_new(spec->source, f, SYNC_ACTION_PUSH);
            if (!plan_tmp) goto error;

            if (list_push(plans, plan_tmp) != LIST_STATUS_OK) goto error;
            plan_tmp = NULL;
        }
    }

    target_files_after = hash_keys(target_hash);
    if (!target_files_after) goto error;

    for (size_t i = 0; i < list_size(target_files_after); i++) {
        char* f = list_get(target_files_after, i);
        if (!hash_get(expected_hash, f)) {
            plan_tmp = sync_plan_new(NULL, f, SYNC_ACTION_RM);
            if (!plan_tmp) goto error;

            if (list_push(plans, plan_tmp) != LIST_STATUS_OK) goto error;
            plan_tmp = NULL;
        }
    }

    plans_sorted = list_sort(plans, sync_plan_cmp);
    if (!plans_sorted) goto error;

    hash_free_deep(target_hash, hash_entry_free_k);
    hash_free_deep(expected_hash, hash_entry_free_k);
    list_free(target_files_after);
    sync_plan_free(plan_tmp);
    list_free(plans);
    return plans_sorted;

error:
    hash_free_deep(target_hash, hash_entry_free_k);
    hash_free_deep(expected_hash, hash_entry_free_k);
    list_free(target_files_after);
    sync_plan_free(plan_tmp);
    list_free_deep(plans, (ListItemFreeFn)sync_plan_free);
    list_free(plans_sorted);
    return NULL;
}

void sync_plan_print(List* plans) {
    for (size_t i = 0; i < list_size(plans); i++) {
        SyncPlan* plan = list_get(plans, i);
        switch (plan->action) {
            case SYNC_ACTION_MKDIR:
                printf("%s: %s/\n", MTP_MKDIR_MSG, plan->target);
                break;
            case SYNC_ACTION_PUSH:
                printf("%s: %s\n", MTP_PUSH_MSG, plan->target);
                break;
            case SYNC_ACTION_RM:
                printf("%s: %s\n", MTP_RM_MSG, plan->target);
                break;
        }
    }
}
