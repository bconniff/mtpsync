#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>

#include "device.h"
#include "list.h"
#include "hash.h"
#include "mtp.h"
#include "str.h"
#include "fs.h"
#include "io.h"

#define MTP_RM_INIT_SIZE 512

static inline int device_file_sort_deletion(const void* a, const void* b) {
    const DeviceFile* aa = *(const DeviceFile**)a;
    const DeviceFile* bb = *(const DeviceFile**)b;

    int cmp = 0;

    // first, if both folders, sort longest path first
    if (aa->is_folder && bb->is_folder) {
        if (!cmp) cmp = str_count_char(bb->path, '/') - str_count_char(aa->path, '/');
    }

    // next, sort folders after files
    if (!cmp) cmp = (aa->is_folder ? 1 : 0) - (bb->is_folder ? 1 : 0);

    // last, sort alphabetically
    if (!cmp) cmp = strcmp(aa->path, bb->path);

    return cmp;
}

static inline void* map_device_file_to_sync_plan(void* item) {
    SyncFile* target = NULL;
    SyncPlan* plan = NULL;

    DeviceFile* df = item;
    target = sync_file_new(df->path, df->is_folder);
    if (!target) goto error;

    plan = sync_plan_new(NULL, target, SYNC_ACTION_RM);
    if (!plan) goto error;

    sync_file_free(target);
    return plan;

error:
    sync_file_free(target);
    sync_plan_free(plan);
    return NULL;
}

MtpStatusCode mtp_rm_files(Device* dev, List* rm_files) {
    MtpStatusCode code = MTP_STATUS_EFAIL;
    List* rm_files_unique = NULL;
    List* rm_files_order = NULL;
    List* plans = NULL;

    if (!list_size(rm_files)) {
        printf("No files to delete.\n");
        code = MTP_STATUS_OK;
        goto done;
    }

    rm_files_unique = hash_unique_strs(rm_files);
    if (!rm_files_unique) goto done;

    printf("Deleting files:\n");

    rm_files_order = list_sort(rm_files_unique, device_file_sort_deletion);
    if (!rm_files_order) goto done;

    plans = list_map(rm_files_order, map_device_file_to_sync_plan);
    if (!plans) goto done;

    sync_plan_print(plans);

    if (!io_confirm("Proceed [y/n]? ")) {
        code = MTP_STATUS_EREJECT;
        goto done;
    }

    if (mtp_execute_sync_plan(dev, plans) != MTP_STATUS_OK) goto done;

    code = MTP_STATUS_OK;

done:
    list_free(rm_files_unique);
    list_free(rm_files_order);
    return code;
}

MtpStatusCode mtp_rm_paths(Device* dev, List* rm_paths) {
    MtpStatusCode code = MTP_STATUS_EFAIL;
    List* tmp_files = NULL;
    List* rm_files = NULL;

    if (device_load(dev) != DEVICE_STATUS_OK) {
        fprintf(stderr, "Failed to load device\n");
        code = MTP_STATUS_EDEVICE;
        goto done;
    }

    rm_files = list_new(MTP_RM_INIT_SIZE);
    if (!rm_files) goto done;

    for (size_t i = 0; i < list_size(rm_paths); i++) {
        char* rm_path = list_get(rm_paths, i);

        tmp_files = device_filter_files(dev, rm_path);
        if (!tmp_files) goto done;

        if (list_push_all(rm_files, tmp_files) != LIST_STATUS_OK) goto done;

        list_free(tmp_files);

        tmp_files = NULL;
    }

    code = mtp_rm_files(dev, rm_files);

done:
    list_free(tmp_files);
    list_free(rm_files);
    return code;
}

static inline MtpStatusCode mtp_rm_callback(Device* dev, void* data) {
    return mtp_rm_paths(dev, (List*)data);
}

MtpStatusCode mtp_rm(MtpDeviceParams* mtp_params, List* rm_paths) {
    MtpStatusCode code = MTP_STATUS_EFAIL;
    char* rm_path_r = NULL;
    List* rm_paths_r = NULL;

    rm_paths_r = list_new(list_size(rm_paths));
    if (!rm_paths_r) goto done;

    for (size_t i = 0; i < list_size(rm_paths); i++) {
        rm_path_r = fs_resolve_cwd("/", list_get(rm_paths, i));
        if (!rm_path_r) goto done;

        if (list_push(rm_paths_r, rm_path_r) != LIST_STATUS_OK) goto done;
        rm_path_r = NULL;
    }

    code = mtp_each_device(mtp_rm_callback, mtp_params, rm_paths_r);

done:
    free(rm_path_r);
    list_free_deep(rm_paths_r, free);
    return code;
};
