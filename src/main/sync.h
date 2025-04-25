#ifndef _SYNC_H_
#define _SYNC_H_

#include "list.h"

// order is important here; it determines which order actions are executed
typedef enum {
    SYNC_ACTION_RM,
    SYNC_ACTION_MKDIR,
    SYNC_ACTION_PUSH,
} SyncAction;

typedef struct {
    char* path;
    int is_folder;
} SyncFile;

typedef struct {
    char* source;
    char* target;
} SyncSpec;

typedef struct {
    SyncFile* source;
    SyncFile* target;
    SyncAction action;
} SyncPlan;

void sync_file_free(SyncFile* f);
SyncFile* sync_file_new(char* path, int is_folder);
void sync_plan_free(SyncPlan* plan);
SyncPlan* sync_plan_new(SyncFile* source, SyncFile* target, SyncAction action);
void sync_spec_free(SyncSpec* spec);
SyncSpec* sync_spec_new(char* source, char* target);

List* sync_plan_create(List* source_files, List* target_files, List* specs, int cleanup);
void sync_plan_print(List* plan);

#endif
