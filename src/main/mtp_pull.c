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

typedef struct {
    char* from_path;
    char* to_path;
} MtpPullParams;

static MtpStatusCode mtp_pull_files(Device* dev, List* pull_files_pre, MtpPullParams* params) {
    MtpStatusCode code = MTP_STATUS_EFAIL;
    List* pull_files = NULL;
    char* to_path = NULL;
    char* to_dir = NULL;

    pull_files = list_new(pull_files_pre->size);
    if (!pull_files) goto done;

    size_t from_path_len = strcmp(params->from_path, "/") == 0 ? 0 : strlen(params->from_path);
    for (size_t i = 0; i < pull_files_pre->size; i++) {
        DeviceFile* f = list_get(pull_files_pre, i);

        to_path = fs_path_join(params->to_path, f->path+from_path_len);
        if (!to_path) goto done;

        struct stat s;
        int stat_code = lstat(to_path, &s);

        if (stat_code == 0) {
            printf("%s: %s\n", MTP_SKIP_MSG, to_path);
        } else if (errno == ENOENT) {
            if (list_push(pull_files, f) != LIST_STATUS_OK) goto done;
            printf("%s: %s\n", MTP_PULL_MSG, to_path);
        } else {
            perror("lstat() failed in mtp_pull_files()");
            goto done;
        }

        free(to_path);
        to_path = NULL;
    }

    if (!pull_files->size) {
        printf("All files already present on the local system.\n");
        code = MTP_STATUS_OK;
        goto done;
    }

    if (!io_confirm("Proceed [y/n]? ")) {
        code = MTP_STATUS_EREJECT;
        goto done;
    }

    for (size_t i = 0; i < pull_files->size; i++) {
        DeviceFile* f = list_get(pull_files, i);

        to_path = fs_path_join(params->to_path, f->path+from_path_len);
        if (!to_path) goto done;

        to_dir = fs_dirname(to_path);
        if (!to_dir) goto done;

        if (fs_mkdirp(to_dir) != FS_STATUS_OK) {
            fprintf(stderr, "fs_mkdirp(%s) failed in mtp_pull_files(): ", to_dir);
            perror(NULL);
            goto done;
        }

        MtpOperationData op = { .name = MTP_PULL_MSG, .file = to_path };
        if (LIBMTP_Get_File_To_File(dev->device, f->id, to_path, mtp_progress, &op) != 0) {
            fprintf(stdout, "\n");
            fprintf(stderr, "Error getting file from MTP device.\n");
            LIBMTP_Dump_Errorstack(dev->device);
            LIBMTP_Clear_Errorstack(dev->device);
            goto done;
        }
        printf("\n");

        free(to_path);
        free(to_dir);

        to_path = NULL;
        to_dir = NULL;
    }

    code = MTP_STATUS_OK;

done:
    free(to_path);
    free(to_dir);
    list_free(pull_files);
    return code;
}

static int exclude_folders(void* item) {
    DeviceFile* file = item;
    return !file->is_folder;
}

static MtpStatusCode mtp_pull_callback(Device* dev, void* data) {
    int code = MTP_STATUS_EFAIL;
    List* pull_files = NULL;
    List* pull_files_only = NULL;

    if (device_load(dev) != DEVICE_STATUS_OK) {
        code = MTP_STATUS_EDEVICE;
        fprintf(stderr, "Failed to load device\n");
        goto done;
    }

    MtpPullParams* params = (MtpPullParams*)data;

    pull_files = device_filter_files(dev, params->from_path);
    if (!pull_files) goto done;

    pull_files_only = list_filter(pull_files, exclude_folders);
    if (!pull_files_only) goto done;

    if (!pull_files_only->size) {
        fprintf(stderr, "No matching files on device: %s\n", params->from_path);
        goto done;
    }

    code = mtp_pull_files(dev, pull_files_only, params);

done:
    list_free(pull_files_only);
    list_free(pull_files);
    return code;
}

MtpStatusCode mtp_pull(MtpDeviceParams* mtp_params, char* from_path, char* to_path) {
    MtpStatusCode code = MTP_STATUS_EFAIL;
    char* from_path_basename = NULL;
    char* to_path_tmp = NULL;

    MtpPullParams pull_params = {
        .from_path = NULL,
        .to_path = NULL
    };

    pull_params.from_path = fs_resolve_cwd("/", from_path);
    if (!pull_params.from_path) goto done;

    if (to_path) {
        pull_params.to_path = fs_resolve(to_path);
        if (!pull_params.to_path) goto done;
    } else {
        if (strcmp("/", pull_params.from_path) == 0) {
            fprintf(stderr, "Destination required when pulling device's root folder\n");
            goto done;
        }

        from_path_basename = fs_basename(pull_params.from_path);
        if (!from_path_basename) goto done;

        to_path_tmp = str_join(2, "./", from_path_basename);
        if (!to_path_tmp) goto done;

        pull_params.to_path = fs_resolve(to_path_tmp);
        if (!pull_params.to_path) goto done;
    }

    code = mtp_each_device(mtp_pull_callback, mtp_params, &pull_params);

done:
    free(pull_params.from_path);
    free(pull_params.to_path);
    free(from_path_basename);
    free(to_path_tmp);
    return code;
};
