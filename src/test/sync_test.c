#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <libgen.h>

#include "../main/sync.h"
#include "../main/list.h"
#include "../main/array.h"
#include "sync_test.h"

typedef struct {
    SyncAction action;
    char* path;
} ExpectedOperation;

static void assert_cleanup(List* source_files, List* target_files, List* specs) {
    ExpectedOperation expected[] = {
        { SYNC_ACTION_RM, "/tgt/four/five/six/31.mp3" },
        { SYNC_ACTION_RM, "/tgt/four/five/six" },
        { SYNC_ACTION_RM, "/tgt/four/five" },
        { SYNC_ACTION_RM, "/tgt/four" },
        { SYNC_ACTION_MKDIR, "/tgt/three" },
        { SYNC_ACTION_MKDIR, "/tgt/test/two" },
        { SYNC_ACTION_MKDIR, "/tgt/test/one/nested" },
        { SYNC_ACTION_MKDIR, "/tgt/test/one/nested/subfolder" },
        { SYNC_ACTION_PUSH, "/tgt/test/one/01.mp3" },
        { SYNC_ACTION_PUSH, "/tgt/test/one/02.mp3" },
        { SYNC_ACTION_PUSH, "/tgt/test/one/nested/subfolder/04.mp3" },
        { SYNC_ACTION_PUSH, "/tgt/test/two/11.mp3" },
        { SYNC_ACTION_PUSH, "/tgt/test/two/12.mp3" },
        { SYNC_ACTION_PUSH, "/tgt/test/two/13.mp3" },
        { SYNC_ACTION_PUSH, "/tgt/three/21.mp3" },
    };

    List* plans = sync_plan_create(source_files, target_files, specs, 1);
    assert(plans);
    assert(ARRAY_LEN(expected) == list_size(plans));
    for (size_t i = 0; i < ARRAY_LEN(expected); i++) {
        SyncPlan* plan = list_get(plans, i);
        assert(expected[i].action == plan->action);
        assert(strcmp(expected[i].path, plan->target->path) == 0);
    }
    list_free_deep(plans, (ListItemFreeFn)sync_plan_free);
}

static void assert_nocleanup(List* source_files, List* target_files, List* specs) {
    ExpectedOperation expected[] = {
        { SYNC_ACTION_MKDIR, "/tgt/three" },
        { SYNC_ACTION_MKDIR, "/tgt/test/two" },
        { SYNC_ACTION_MKDIR, "/tgt/test/one/nested" },
        { SYNC_ACTION_MKDIR, "/tgt/test/one/nested/subfolder" },
        { SYNC_ACTION_PUSH, "/tgt/test/one/01.mp3" },
        { SYNC_ACTION_PUSH, "/tgt/test/one/02.mp3" },
        { SYNC_ACTION_PUSH, "/tgt/test/one/nested/subfolder/04.mp3" },
        { SYNC_ACTION_PUSH, "/tgt/test/two/11.mp3" },
        { SYNC_ACTION_PUSH, "/tgt/test/two/12.mp3" },
        { SYNC_ACTION_PUSH, "/tgt/test/two/13.mp3" },
        { SYNC_ACTION_PUSH, "/tgt/three/21.mp3" },
    };

    List* plans = sync_plan_create(source_files, target_files, specs, 0);
    assert(plans);
    assert(ARRAY_LEN(expected) == list_size(plans));
    for (size_t i = 0; i < ARRAY_LEN(expected); i++) {
        SyncPlan* plan = list_get(plans, i);
        assert(expected[i].action == plan->action);
        assert(strcmp(expected[i].path, plan->target->path) == 0);
    }
    list_free_deep(plans, (ListItemFreeFn)sync_plan_free);
}

int sync_test(int sz) {
    char* source_file_paths[] = {
        "/src/test/one/01.mp3",
        "/src/test/one/02.mp3",
        "/src/test/one/03.mp3",
        "/src/test/one/nested/subfolder/04.mp3",
        "/src/test/two/11.mp3",
        "/src/test/two/12.mp3",
        "/src/test/two/13.mp3",
        "/src/three/21.mp3",
    };

    char* target_file_paths[] = {
        "/tgt/test/one/03.mp3",
        "/tgt/four/five/six/31.mp3",
    };

    List* source_files = list_new(ARRAY_LEN(source_file_paths));
    List* target_files = list_new(ARRAY_LEN(target_file_paths));
    List* specs = list_new(ARRAY_LEN(source_file_paths));
    assert(source_files && target_files && specs);

    for (size_t i = 0; i < ARRAY_LEN(source_file_paths); i++) {
        char* source = source_file_paths[i];
        char* target = malloc((strlen(source) + 1) * sizeof(char));
        assert(target);

        sprintf(target, "/tgt%s", source+4);

        assert(list_push(source_files, sync_file_new(source_file_paths[i], 0)) == LIST_STATUS_OK);
        assert(list_push(specs, sync_spec_new(source, target)) == LIST_STATUS_OK);
        free(target);
    }

    for (size_t i = 0; i < ARRAY_LEN(target_file_paths); i++) {
        assert(list_push(target_files, sync_file_new(target_file_paths[i], 0)) == LIST_STATUS_OK);
    }

    assert_cleanup(source_files, target_files, specs);
    assert_nocleanup(source_files, target_files, specs);

    list_free_deep(source_files, (ListItemFreeFn)sync_file_free);
    list_free_deep(target_files, (ListItemFreeFn)sync_file_free);
    list_free_deep(specs, (ListItemFreeFn)sync_spec_free);

    return 0;
}
