#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "device.h"
#include "list.h"
#include "mtp.h"
#include "mtp_rm.h"
#include "str.h"
#include "fs.h"
#include "io.h"
#include "hash.h"
#include "sync.h"
#include "array.h"
#include "map.h"

#define MTP_PUSH_LIST_INIT_SIZE 512

typedef struct {
    int cleanup;
    List* push_specs;
    char* to_path;
} MtpSyncSpec;

static inline int str_sort_alpha(const void* a, const void* b) {
    const char* aa = *(const char**)a;
    const char* bb = *(const char**)b;
    return strcmp(aa, bb);
}

static MtpStatusCode mtp_push_callback(Device* dev, void* data) {
    MtpStatusCode code = MTP_STATUS_EFAIL;
    List* plans = NULL;
    List* device_files = NULL;
    List* source_files = NULL;
    List* target_files = NULL;
    List* push_specs = NULL;
    char* dname = NULL;

    if (device_load(dev) != DEVICE_STATUS_OK) {
        code = MTP_STATUS_EDEVICE;
        fprintf(stderr, "Failed to load device\n");
        goto done;
    }

    MtpSyncSpec* sync_spec = (MtpSyncSpec*)data;

    device_files = device_filter_files(dev, sync_spec->to_path);
    if (!device_files) goto done;

    MapStatusCode map_status = MTP_STATUS_OK;
    source_files = list_map_data(sync_spec->push_specs, (ListMapDataFn)map_sync_spec_to_sync_file, &map_status);
    if (!source_files || (map_status != MAP_STATUS_OK)) goto done;

    target_files = list_map_data(device_files, (ListMapDataFn)map_device_file_to_sync_file, &map_status);
    if (!target_files || (map_status != MAP_STATUS_OK)) goto done;

    plans = sync_plan_push(source_files, target_files, sync_spec->push_specs, sync_spec->cleanup);
    if (!plans) goto done;

    sync_plan_print(plans);

    if (list_size(plans)) {
        if (!io_confirm("Proceed [y/n]? ")) {
            code = MTP_STATUS_EREJECT;
            goto done;
        }
        if (mtp_execute_sync_plan(dev, plans) != MTP_STATUS_OK) goto done;
    } else {
        printf("All files already present on the device.\n");
    }

    code = MTP_STATUS_OK;

done:
    free(dname);
    list_free(device_files);
    list_free_deep(source_files, (ListItemFreeFn)sync_file_free);
    list_free_deep(target_files, (ListItemFreeFn)sync_file_free);
    list_free_deep(plans, (ListItemFreeFn)sync_plan_free);
    list_free(push_specs);
    return code;
}

MtpStatusCode mtp_push(MtpDeviceParams* mtp_params, char* from_path, char* to_path, int cleanup) {
    MtpStatusCode code = MTP_STATUS_EFAIL;
    SyncSpec* push_spec = NULL;
    List* push_specs = NULL;
    List* files = NULL;
    List* files_sorted = NULL;
    char* from_path_r = NULL;
    char* to_path_r = NULL;
    char* target = NULL;

    push_specs = list_new(MTP_PUSH_LIST_INIT_SIZE);
    if (!push_specs) goto done;

    from_path_r = fs_resolve(from_path);
    if (!from_path_r) goto done;

    to_path_r = fs_resolve_cwd("/", to_path);
    if (!to_path_r) goto done;

    files = fs_collect_files(from_path_r);
    if (!files) goto done;

    if (!list_size(files)) {
        printf("No files in local path: %s\n", from_path_r);
        goto done;
    }

    files_sorted = list_sort(files, str_sort_alpha);
    if (!files_sorted) goto done;

    size_t from_path_len = strlen(from_path_r);
    for (size_t i = 0; i < list_size(files_sorted); i++) {
        char* source = list_get(files_sorted, i);

        target = fs_path_join(to_path_r, source + from_path_len);
        if (!target) goto done;

        push_spec = sync_spec_new(source, target);
        if (!push_spec) goto done;

        if (list_push(push_specs, push_spec) != LIST_STATUS_OK) goto done;

        free(target);

        push_spec = NULL;
        target = NULL;
    }

    MtpSyncSpec sync_spec = {
        .push_specs = push_specs,
        .to_path = to_path_r,
        .cleanup = cleanup
    };
    code = mtp_each_device(mtp_push_callback, mtp_params, &sync_spec);

done:
    sync_spec_free(push_spec);
    free(target);
    free(from_path_r);
    free(to_path_r);
    list_free(files_sorted);
    list_free_deep(files, free);
    list_free_deep(push_specs, (ListItemFreeFn)sync_spec_free);
    return code;
}
