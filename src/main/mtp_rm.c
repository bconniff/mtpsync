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
#include "map.h"

#define MTP_RM_INIT_SIZE 512

MtpStatusCode mtp_rm_files(Device* dev, List* rm_files) {
    MtpStatusCode code = MTP_STATUS_EFAIL;
    List* sync_files = NULL;
    List* plans = NULL;

    MapStatusCode map_status = MTP_STATUS_OK;
    sync_files = list_map_data(rm_files, (ListMapDataFn)map_device_file_to_sync_file, &map_status);
    if (!sync_files || (map_status != MAP_STATUS_OK)) goto done;

    plans = sync_plan_rm(sync_files);
    if (!plans) goto done;

    if (!list_size(plans)) {
        printf("No files to delete.\n");
        code = MTP_STATUS_OK;
        goto done;
    }

    sync_plan_print(plans, MTP_PUSH_MSG);

    if (!io_confirm("Proceed [y/n]? ")) {
        code = MTP_STATUS_EREJECT;
        goto done;
    }

    if (mtp_execute_push_plan(dev, plans) != MTP_STATUS_OK) goto done;

    code = MTP_STATUS_OK;

done:
    list_free_deep(sync_files, (ListItemFreeFn)sync_file_free);
    list_free_deep(plans, (ListItemFreeFn)sync_plan_free);
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
