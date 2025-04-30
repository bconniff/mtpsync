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

#define MTP_PUSH_LIST_INIT_SIZE 512

typedef struct {
    int cleanup;
    List* source_files;
    List* push_specs;
    char* to_path;
} MtpPushParams;

static MtpStatusCode mtp_push_callback(Device* dev, void* data) {
    MtpStatusCode code = MTP_STATUS_EFAIL;
    List* plans = NULL;
    List* target_files = NULL;

    if (device_load(dev) != DEVICE_STATUS_OK) {
        code = MTP_STATUS_EDEVICE;
        fprintf(stderr, "Failed to load device\n");
        goto done;
    }

    MtpPushParams* params = (MtpPushParams*)data;

    target_files = device_filter_files(dev, params->to_path);
    if (!target_files) goto done;

    plans = sync_plan_push(params->source_files, target_files, params->push_specs, params->cleanup);
    if (!plans) goto done;

    sync_plan_print(plans, MTP_PUSH_MSG);

    if (list_size(plans)) {
        if (!io_confirm("Proceed [y/n]? ")) {
            code = MTP_STATUS_EREJECT;
            goto done;
        }
        if (mtp_execute_push_plan(dev, plans) != MTP_STATUS_OK) goto done;
    } else {
        printf("All files already present on the device.\n");
    }

    code = MTP_STATUS_OK;

done:
    list_free(target_files);
    list_free_deep(plans, (ListItemFreeFn)sync_plan_free);
    return code;
}

MtpStatusCode mtp_push(MtpDeviceParams* mtp_params, char* from_path, char* to_path, int cleanup) {
    MtpStatusCode code = MTP_STATUS_EFAIL;
    List* source_files = NULL;
    List* push_specs = NULL;
    char* from_path_r = NULL;
    char* to_path_r = NULL;

    from_path_r = fs_resolve(from_path);
    if (!from_path_r) goto done;

    to_path_r = fs_resolve_cwd("/", to_path);
    if (!to_path_r) goto done;

    source_files = fs_collect_files(from_path_r);
    if (!source_files) goto done;

    if (!list_size(source_files)) {
        printf("No files in local path: %s\n", from_path_r);
        goto done;
    }

    push_specs = sync_spec_create(source_files, from_path_r, to_path_r);
    if (!push_specs) goto done;

    MtpPushParams params = {
        .source_files = source_files,
        .push_specs = push_specs,
        .to_path = to_path_r,
        .cleanup = cleanup
    };
    code = mtp_each_device(mtp_push_callback, mtp_params, &params);

done:
    free(from_path_r);
    free(to_path_r);
    list_free_deep(source_files, (ListItemFreeFn)file_free);
    list_free_deep(push_specs, (ListItemFreeFn)sync_spec_free);
    return code;
}
