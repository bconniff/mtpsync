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

typedef struct {
    List* plans;
    SyncFile* source;
} SyncAncestorData;

typedef SyncStatusCode (*SyncAncestorFn)(SyncFile* target, int is_ancestor, void* data);

static inline int sync_plan_cmp(const void* a, const void* b) {
    const SyncPlan* aa = *(const SyncPlan**)a;
    const SyncPlan* bb = *(const SyncPlan**)b;

    char* aa_target = aa->target->path;
    char* bb_target = bb->target->path;

    int cmp = 0;

    // first sort by action
    if (!cmp) cmp = aa->action - bb->action;

    // action-specific sorting
    if (!cmp) {
        switch (aa->action) {
            // for deletion, sort longest paths first
            case SYNC_ACTION_RM:
                cmp = str_count_char(bb_target, '/') - str_count_char(aa_target, '/');
                break;

            // for new dirs, sort shortest paths first
            case SYNC_ACTION_MKDIR:
                cmp = str_count_char(aa_target, '/') - str_count_char(bb_target, '/');
                break;

            default:
        }
    }

    // last, sort alphabetically
    if (!cmp) cmp = strcmp(aa_target, bb_target);

    return cmp;
}

void sync_file_free(SyncFile* f) {
    if (f) {
        free(f->path);
        free(f);
    }
}

void sync_file_hash_entry_free(HashEntry* e) {
    sync_file_free(hash_entry_value(e));
    hash_entry_free(e);
}

SyncFile* sync_file_new(char* path, int is_folder) {
    char* path_dup = NULL;
    SyncFile* file = NULL;

    path_dup = strdup(path);
    if (!path_dup) goto error;

    file = malloc(sizeof(SyncFile));
    if (!file) goto error;
    file->path = path_dup;
    file->is_folder = is_folder;
    return file;

error:
    free(path_dup);
    free(file);
    return NULL;
}

SyncFile* sync_file_dup(SyncFile* f) {
    return f ? sync_file_new(f->path, f->is_folder) : NULL;
}

void sync_plan_free(SyncPlan* plan) {
    if (plan) {
        sync_file_free(plan->source);
        sync_file_free(plan->target);
        free(plan);
    }
}

SyncPlan* sync_plan_new(SyncFile* source, SyncFile* target, SyncAction action) {
    SyncPlan* plan = NULL;
    SyncFile* source_dup = NULL;
    SyncFile* target_dup = NULL;

    if (source) {
        source_dup = sync_file_dup(source);
        if (!source_dup) goto error;
    }

    if (target) {
        target_dup = sync_file_dup(target);
        if (!target_dup) goto error;
    }

    plan = malloc(sizeof(SyncPlan));
    if (!plan) goto error;
    plan->source = source_dup;
    plan->target = target_dup;
    plan->action = action;
    return plan;

error:
    sync_file_free(source_dup);
    sync_file_free(target_dup);
    free(plan);
    return NULL;
}

void sync_spec_free(SyncSpec* spec) {
    if (spec) {
        free(spec->source);
        free(spec->target);
        free(spec);
    }
}

SyncSpec* sync_spec_new(char* source, char* target) {
    SyncSpec* spec = NULL;
    char* source_dup = NULL;
    char* target_dup = NULL;

    source_dup = strdup(source);
    if (!source_dup) goto error;

    target_dup = strdup(target);
    if (!target_dup) goto error;

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

static SyncStatusCode put_ancestors(Hash* hash, SyncFile* f, SyncAncestorFn fn, void* data) {
    int code = SYNC_STATUS_EFAIL;
    SyncFile* target = NULL;
    char* target_path = NULL;

    target_path = strdup(f->path);
    if (!target_path) goto done;

    int is_ancestor = 0;

    while (!hash_get(hash, target_path)) {
        target = sync_file_new(target_path, is_ancestor || f->is_folder);

        if (!target) goto done;

        if (fn && (fn(target, is_ancestor, data) != SYNC_STATUS_OK)) goto done;

        HashPutResult r = hash_put(hash, target->path, target);
        sync_file_hash_entry_free(r.old_entry);
        if (r.status != HASH_STATUS_OK) goto done;

        target = NULL;

        char* old_target_path = target_path;
        target_path = fs_dirname(target_path);
        free(old_target_path);
        if (!target_path) goto done;

        is_ancestor = 1;
    }

    code = SYNC_STATUS_OK;

done:
    sync_file_free(target);
    free(target_path);
    return code;
}

static inline SyncStatusCode ancestor_cb(SyncFile* target, int is_ancestor, void* data) {
    SyncPlan* plan = NULL;

    SyncAncestorData* d = data;
    SyncFile* source = is_ancestor ? NULL : d->source;
    SyncAction action = target->is_folder ? SYNC_ACTION_MKDIR : SYNC_ACTION_PUSH;

    plan = sync_plan_new(source, target, action);
    if (!plan) goto error;

    if (list_push(d->plans, plan) != LIST_STATUS_OK) goto error;

    return SYNC_STATUS_OK;

error:
    sync_plan_free(plan);
    return SYNC_STATUS_EFAIL;
}

Hash* hash_of_files(List* files) {
    Hash* hash = NULL;

    hash = hash_new_str(list_size(files) * 2);
    if (!hash) goto error;

    for (size_t i = 0; i < list_size(files); i++) {
        SyncFile* f = list_get(files, i);
        if (put_ancestors(hash, f, NULL, NULL) != SYNC_STATUS_OK) goto error;
    }

    return hash;

error:
    hash_free_deep(hash, sync_file_hash_entry_free);
    return NULL;
}

List* sync_plan_create(List* source_files, List* target_files, List* specs, int cleanup) {
    Hash* source_hash = NULL;
    Hash* target_hash = NULL;
    Hash* expected_hash = NULL;
    List* target_files_after = NULL;
    List* plans = NULL;
    List* plans_sorted = NULL;
    SyncFile* file_tmp = NULL;
    SyncPlan* plan_tmp = NULL;

    plans = list_new(list_size(target_files) + list_size(source_files));
    if (!plans) goto error;

    target_hash = hash_of_files(target_files);
    if (!target_hash) goto error;

    source_hash = hash_of_files(source_files);
    if (!source_hash) goto error;

    expected_hash = hash_new_str(list_size(specs) * 2);
    if (!expected_hash) goto error;

    for (size_t i = 0; i < list_size(specs); i++) {
        SyncSpec* spec = list_get(specs, i);
        SyncFile* f = hash_get(source_hash, spec->source);

        if (!f) goto error;

        file_tmp = sync_file_new(spec->target, f->is_folder);
        if (!file_tmp) goto error;

        SyncAncestorData data = {
            .source = f,
            .plans = plans,
        };

        if (put_ancestors(expected_hash, file_tmp, NULL, NULL) != SYNC_STATUS_OK) goto error;
        if (put_ancestors(target_hash, file_tmp, ancestor_cb, &data) != SYNC_STATUS_OK) goto error;

        sync_file_free(file_tmp);
        file_tmp = NULL;
    }

    if (cleanup) {
        target_files_after = hash_values(target_hash);
        if (!target_files_after) goto error;

        for (size_t i = 0; i < list_size(target_files_after); i++) {
            SyncFile* f = list_get(target_files_after, i);
            if (strcmp(f->path, "/") != 0 && !hash_get(expected_hash, f->path)) {
                plan_tmp = sync_plan_new(NULL, f, SYNC_ACTION_RM);
                if (!plan_tmp) goto error;

                if (list_push(plans, plan_tmp) != LIST_STATUS_OK) goto error;
                plan_tmp = NULL;
            }
        }
    }

    plans_sorted = list_sort(plans, sync_plan_cmp);
    if (!plans_sorted) goto error;

    goto done; // success

error:
    list_free_deep(plans, (ListItemFreeFn)sync_plan_free);
    plans = NULL; // don't double-free

done:
    hash_free_deep(target_hash, sync_file_hash_entry_free);
    hash_free_deep(source_hash, sync_file_hash_entry_free);
    hash_free_deep(expected_hash, sync_file_hash_entry_free);
    list_free(target_files_after);
    sync_file_free(file_tmp);
    sync_plan_free(plan_tmp);
    list_free(plans);
    return plans_sorted;
}

void sync_plan_print(List* plans) {
    for (size_t i = 0; i < list_size(plans); i++) {
        SyncPlan* plan = list_get(plans, i);
        switch (plan->action) {
            case SYNC_ACTION_MKDIR:
                printf("%s: %s/\n", MTP_MKDIR_MSG, plan->target->path);
                break;
            case SYNC_ACTION_PUSH:
                printf("%s: %s\n", MTP_PUSH_MSG, plan->target->path);
                break;
            case SYNC_ACTION_RM:
                printf("%s: %s%s\n", MTP_RM_MSG, plan->target->path, plan->target->is_folder ? "/" : "");
                break;
        }
    }
}
