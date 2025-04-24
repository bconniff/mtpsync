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
    char* source;
    char* target;
} SyncSpec;

typedef struct {
    char* source;
    char* target;
    SyncAction action;
} SyncPlan;

SyncSpec* sync_spec_new(char* source, char* target);
void sync_spec_free(SyncSpec* plan);

SyncPlan* sync_plan_new(char* source, char* target, SyncAction a);
void sync_plan_free(SyncPlan* plan);
List* sync_plan(List* specs, List* target_files);
void sync_plan_print(List* plan);

#endif
