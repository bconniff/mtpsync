#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <errno.h>
#include <sys/stat.h>

#include "device.h"
#include "list.h"
#include "mtp.h"
#include "str.h"
#include "fs.h"
#include "io.h"
#include "map.h"

typedef struct {
    int cleanup;
    char* from_path;
    char* to_path;
} MtpPullParams;

static List* collect_local_files(char* path) {
    List* child_files = NULL;
    List* parent_dirs = NULL;
    List* local_files = NULL;
    SyncFile* file = NULL;

    child_files = fs_collect_files(path);
    if (!child_files && errno == ENOENT) child_files = list_new(0);
    if (!child_files) goto error;

    parent_dirs = fs_collect_ancestors(path);
    if (!parent_dirs) goto error;

    local_files = list_new(list_size(child_files) + list_size(parent_dirs));
    if (!local_files) goto error;

    for (size_t i = 0; i < list_size(parent_dirs); i++) {
        file = sync_file_new(list_get(parent_dirs, i), 1);
        if (!file) goto error;
        if (list_push(local_files, file) != LIST_STATUS_OK) goto error;
        file = NULL;
    }

    for (size_t i = 0; i < list_size(child_files); i++) {
        file = sync_file_new(list_get(child_files, i), 0);
        if (!file) goto error;
        if (list_push(local_files, file) != LIST_STATUS_OK) goto error;
        file = NULL;
    }

    goto done;

error:
    list_free_deep(local_files, (ListItemFreeFn)sync_file_free);
    local_files = NULL;

done:
    free(file);
    list_free_deep(child_files, free);
    list_free_deep(parent_dirs, free);
    return local_files;
}

static MtpStatusCode mtp_pull_callback(Device* dev, void* data) {
    MtpStatusCode code = MTP_STATUS_EFAIL;
    List* device_files = NULL;
    List* local_files = NULL;
    List* pull_specs = NULL;
    List* source_files = NULL;
    List* plans = NULL;

    MtpPullParams* params = (MtpPullParams*)data;

    if (device_load(dev) != DEVICE_STATUS_OK) {
        code = MTP_STATUS_EDEVICE;
        fprintf(stderr, "Failed to load device\n");
        goto done;
    }

    device_files = device_filter_files(dev, params->from_path);
    if (!device_files) goto done;

    MapStatusCode map_status = MAP_STATUS_OK;
    source_files = list_map_data(device_files, (ListMapDataFn)map_device_file_to_sync_file, &map_status);
    if (!source_files || (map_status != MAP_STATUS_OK)) goto done;

    local_files = collect_local_files(params->to_path);
    if (!local_files) goto done;

    pull_specs = sync_spec_create(source_files, params->from_path, params->to_path);
    if (!pull_specs) goto done;

    plans = sync_plan_push(source_files, local_files, pull_specs, params->cleanup);
    if (!plans) goto done;

    sync_plan_print(plans, MTP_PULL_MSG);

    if (list_size(plans)) {
        if (!io_confirm("Proceed [y/n]? ")) {
            code = MTP_STATUS_EREJECT;
            goto done;
        }
        if (mtp_execute_pull_plan(dev, plans) != MTP_STATUS_OK) goto done;
    } else {
        printf("All files already present on the local system.\n");
    }

    code = MTP_STATUS_OK;

done:
    list_free(device_files);
    list_free_deep(local_files, free);
    list_free_deep(source_files, (ListItemFreeFn)sync_file_free);
    list_free_deep(plans, (ListItemFreeFn)sync_plan_free);
    list_free_deep(pull_specs, (ListItemFreeFn)sync_spec_free);
    return code;
}

MtpStatusCode mtp_pull(MtpDeviceParams* mtp_params, char* from_path, char* to_path, int cleanup) {
    MtpStatusCode code = MTP_STATUS_EFAIL;
    char* from_path_bname = NULL;
    char* to_path_tmp = NULL;
    char* from_path_r = NULL;
    char* to_path_r = NULL;

    from_path_r = fs_resolve_cwd("/", from_path);
    if (!from_path_r) goto done;

    if (to_path) {
        to_path_r = fs_resolve(to_path);
        if (!to_path_r) goto done;
    } else {
        if (strcmp("/", from_path_r) == 0) {
            fprintf(stderr, "Destination required when pulling device's root folder\n");
            goto done;
        }

        from_path_bname = fs_basename(from_path_r);
        if (!from_path_bname) goto done;

        to_path_tmp = str_join(2, "./", from_path_bname);
        if (!to_path_tmp) goto done;

        to_path_r = fs_resolve(to_path_tmp);
        if (!to_path_r) goto done;
    }

    MtpPullParams pull_params = {
        .from_path = from_path_r,
        .to_path = to_path_r,
        .cleanup = cleanup,
    };
    code = mtp_each_device(mtp_pull_callback, mtp_params, &pull_params);

done:
    free(from_path_r);
    free(to_path_r);
    free(from_path_bname);
    free(to_path_tmp);
    return code;
};
