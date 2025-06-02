#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <errno.h>
#include <sys/stat.h>

#include "device.h"
#include "file.h"
#include "list.h"
#include "mtp.h"
#include "str.h"
#include "fs.h"
#include "io.h"

typedef struct {
    MtpArgs* args;
    char* from_path;
    char* to_path;
} MtpPullParams;

static List* collect_local_files(char* path) {
    List* child_files = NULL;
    List* parent_dirs = NULL;
    List* all_files = NULL;

    child_files = fs_collect_files(path);
    if (!child_files && errno == ENOENT) child_files = list_new(0);
    if (!child_files) goto error;

    parent_dirs = fs_collect_ancestors(path);
    if (!parent_dirs) goto error;

    all_files = list_new(list_size(child_files) + list_size(parent_dirs));
    if (!all_files) goto error;

    if (list_push_all(all_files, parent_dirs) != LIST_STATUS_OK) goto error;
    if (list_push_all(all_files, child_files) != LIST_STATUS_OK) goto error;

    goto done;

done:
    list_free(child_files);
    list_free(parent_dirs);
    return all_files;

error:
    list_free_deep(all_files, (ListItemFreeFn)child_files);
    list_free_deep(all_files, (ListItemFreeFn)parent_dirs);
    list_free(all_files);
    return NULL;
}

static MtpStatusCode mtp_pull_callback(Device* dev, void* data) {
    MtpStatusCode code = MTP_STATUS_EFAIL;
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

    source_files = device_filter_files(dev, params->from_path);
    if (!source_files) goto done;

    local_files = collect_local_files(params->to_path);
    if (!local_files) goto done;

    pull_specs = sync_spec_create(source_files, params->from_path, params->to_path);
    if (!pull_specs) goto done;

    plans = sync_plan_push(source_files, local_files, pull_specs, params->args->cleanup);
    if (!plans) goto done;

    if (list_size(plans)) {
        int yes = params->args->yes;
        if (!yes) {
            sync_plan_print(plans, MTP_PULL_MSG);
            yes = io_confirm("Proceed [y/n]? ");
        }

        if (!yes) {
            code = MTP_STATUS_EREJECT;
            goto done;
        }

        if (mtp_execute_pull_plan(dev, plans) != MTP_STATUS_OK) goto done;
    } else {
        printf("All files already present on the local system.\n");
    }

    code = MTP_STATUS_OK;

done:
    list_free(source_files);
    list_free_deep(local_files, (ListItemFreeFn)file_free);
    list_free_deep(plans, (ListItemFreeFn)sync_plan_free);
    list_free_deep(pull_specs, (ListItemFreeFn)sync_spec_free);
    return code;
}

MtpStatusCode mtp_pull(MtpArgs* args, char* from_path, char* to_path) {
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
        .args = args,
        .from_path = from_path_r,
        .to_path = to_path_r,
    };
    code = mtp_each_device(mtp_pull_callback, args, &pull_params);

done:
    free(from_path_r);
    free(to_path_r);
    free(from_path_bname);
    free(to_path_tmp);
    return code;
};
