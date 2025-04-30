#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>

#include "device.h"
#include "list.h"
#include "mtp.h"
#include "str.h"
#include "fs.h"
#include "io.h"

static MtpStatusCode mtp_ls_callback(Device* dev, void* ls_path) {
    MtpStatusCode code = MTP_STATUS_EFAIL;
    List* ls_files = NULL;

    if (device_load(dev) != DEVICE_STATUS_OK) {
        code = MTP_STATUS_EDEVICE;
        fprintf(stderr, "Failed to load device\n");
        goto done;
    }

    ls_files = device_filter_files(dev, ls_path);
    if (!ls_files) goto done;

    for (size_t i = 0; i < list_size(ls_files); i++) {
        File* file = list_get(ls_files, i);
        printf("%s%s\n", file->path, file->is_folder ? "/" : "");
    }

    code = MTP_STATUS_OK;

done:
    list_free(ls_files);
    return code;
}

MtpStatusCode mtp_ls(MtpDeviceParams* mtp_params, char* ls_path) {
    MtpStatusCode code = MTP_STATUS_EFAIL;
    char* ls_path_r = NULL;

    ls_path_r = fs_resolve_cwd("/", ls_path);
    if (!ls_path_r) goto done;

    code = mtp_each_device(mtp_ls_callback, mtp_params, ls_path_r);

done:
    free(ls_path_r);
    return code;
};
