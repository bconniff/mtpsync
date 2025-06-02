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

typedef struct {
    MtpArgs* args;
    List* rm_paths;
} MtpRmParams;

static MtpStatusCode mtp_rm_files(Device* dev, MtpArgs* args, List* rm_files) {
    MtpStatusCode code = MTP_STATUS_EFAIL;
    List* plans = NULL;

    plans = sync_plan_rm(rm_files);
    if (!plans) goto done;

    if (!list_size(plans)) {
        printf("No files to delete.\n");
        code = MTP_STATUS_OK;
        goto done;
    }

    int yes = args->yes;
    if (!yes) {
        sync_plan_print(plans, MTP_PUSH_MSG);
        yes = io_confirm("Proceed [y/n]? ");
    }

    if (!yes) {
        code = MTP_STATUS_EREJECT;
        goto done;
    }

    if (mtp_execute_push_plan(dev, plans) != MTP_STATUS_OK) goto done;

    code = MTP_STATUS_OK;

done:
    list_free_deep(plans, (ListItemFreeFn)sync_plan_free);
    return code;
}

static MtpStatusCode mtp_rm_paths(Device* dev, MtpArgs* args, List* rm_paths) {
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

    code = mtp_rm_files(dev, args, rm_files);

done:
    list_free(tmp_files);
    list_free(rm_files);
    return code;
}

static inline MtpStatusCode mtp_rm_callback(Device* dev, void* data) {
    MtpRmParams* params = data;
    return mtp_rm_paths(dev, params->args, params->rm_paths);
}

MtpStatusCode mtp_rm(MtpArgs* args, List* rm_paths) {
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

    MtpRmParams rm_params = {
        .args = args,
        .rm_paths = rm_paths,
    };
    code = mtp_each_device(mtp_rm_callback, args, &rm_params);

done:
    free(rm_path_r);
    list_free_deep(rm_paths_r, free);
    return code;
};
