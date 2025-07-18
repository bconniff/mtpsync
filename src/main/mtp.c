#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <libmtp.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "device.h"
#include "mtp.h"
#include "str.h"
#include "fs.h"
#include "list.h"
#include "array.h"

typedef struct {
    MtpStatusCode status;
    LIBMTP_raw_device_t* devices;
    int count;
} MtpRawDevices;

typedef struct {
    char* extension;
    LIBMTP_filetype_t type;
} MtpPushFileType;

MtpPushFileType file_types[] = {
    { ".wav", LIBMTP_FILETYPE_WAV },
    { ".mp3", LIBMTP_FILETYPE_MP3 },
    { ".wma", LIBMTP_FILETYPE_WMA },
    { ".ogg", LIBMTP_FILETYPE_OGG },
    { ".mp4", LIBMTP_FILETYPE_MP4 },
    { ".wmv", LIBMTP_FILETYPE_WMV },
    { ".avi", LIBMTP_FILETYPE_AVI },
    { ".mpeg", LIBMTP_FILETYPE_MPEG },
    { ".mpg", LIBMTP_FILETYPE_MPEG },
    { ".asf", LIBMTP_FILETYPE_ASF },
    { ".qt", LIBMTP_FILETYPE_QT },
    { ".mov", LIBMTP_FILETYPE_QT },
    { ".wma", LIBMTP_FILETYPE_WMA },
    { ".jpg", LIBMTP_FILETYPE_JPEG },
    { ".jpeg", LIBMTP_FILETYPE_JPEG },
    { ".jfif", LIBMTP_FILETYPE_JFIF },
    { ".tif", LIBMTP_FILETYPE_TIFF },
    { ".tiff", LIBMTP_FILETYPE_TIFF },
    { ".bmp", LIBMTP_FILETYPE_BMP },
    { ".gif", LIBMTP_FILETYPE_GIF },
    { ".pic", LIBMTP_FILETYPE_PICT },
    { ".pict", LIBMTP_FILETYPE_PICT },
    { ".png", LIBMTP_FILETYPE_PNG },
    { ".wmf", LIBMTP_FILETYPE_WINDOWSIMAGEFORMAT },
    { ".ics", LIBMTP_FILETYPE_VCALENDAR2 },
    { ".exe", LIBMTP_FILETYPE_WINEXEC },
    { ".com", LIBMTP_FILETYPE_WINEXEC },
    { ".bat", LIBMTP_FILETYPE_WINEXEC },
    { ".dll", LIBMTP_FILETYPE_WINEXEC },
    { ".sys", LIBMTP_FILETYPE_WINEXEC },
    { ".aac", LIBMTP_FILETYPE_AAC },
    { ".mp2", LIBMTP_FILETYPE_MP2 },
    { ".flac", LIBMTP_FILETYPE_FLAC },
    { ".m4a", LIBMTP_FILETYPE_M4A },
    { ".doc", LIBMTP_FILETYPE_DOC },
    { ".xml", LIBMTP_FILETYPE_XML },
    { ".xls", LIBMTP_FILETYPE_XLS },
    { ".ppt", LIBMTP_FILETYPE_PPT },
    { ".mht", LIBMTP_FILETYPE_MHT },
    { ".jp2", LIBMTP_FILETYPE_JP2 },
    { ".jpx", LIBMTP_FILETYPE_JPX },
    { ".bin", LIBMTP_FILETYPE_FIRMWARE },
    { ".vcf", LIBMTP_FILETYPE_VCARD3 },
};

static void mtp_init_once() {
    static int is_init = 0;
    if (!is_init) {
        is_init = 1;
        LIBMTP_Init();
    }
}

static MtpRawDevices mtp_detect_raw_devices() {
    MtpRawDevices result = {
        .status = MTP_STATUS_EFAIL,
        .devices = NULL,
        .count = 0,
    };

    LIBMTP_error_number_t err = LIBMTP_Detect_Raw_Devices(&result.devices, &result.count);

    switch (err) {
        case LIBMTP_ERROR_NO_DEVICE_ATTACHED:
            result.status = MTP_STATUS_OK;
            break;

        case LIBMTP_ERROR_NONE:
            result.status = MTP_STATUS_OK;
            break;

        case LIBMTP_ERROR_CONNECTING:
            result.status = MTP_STATUS_EDEVICE;
            break;

        case LIBMTP_ERROR_MEMORY_ALLOCATION:
            result.status = MTP_STATUS_ENOMEM;
            break;

        default:
            result.status = MTP_STATUS_EFAIL;
            break;
    }

    return result;
}

static void mtp_release_device(LIBMTP_mtpdevice_t* device) {
    if (device) LIBMTP_Release_Device(device);
}

static LIBMTP_mtpdevice_t* mtp_open_raw_device(LIBMTP_raw_device_t* raw_device, int i) {
    LIBMTP_mtpdevice_t* device = NULL;

    device = LIBMTP_Open_Raw_Device_Uncached(raw_device);
    if (!device) {
        fprintf(stderr, "Unable to open raw device %d\n", i);
        goto error;
    }

    if (LIBMTP_Get_Errorstack(device)) {
        LIBMTP_Dump_Errorstack(device);
        LIBMTP_Clear_Errorstack(device);
    }

    if (LIBMTP_Get_Storage(device, LIBMTP_STORAGE_SORTBY_NOTSORTED) != 0) {
        perror("Could not load device storage");
        goto error;
    }

    return device;

error:
    mtp_release_device(device);
    return NULL;
}

static inline int mtp_progress(const uint64_t sent, const uint64_t total, void const * const data) {
    MtpOperationData* op = (MtpOperationData*)data;
    int percent = (sent*100)/total;
    printf("\33[2K\r%s: %s: %d%%", op->name, op->file, percent);
    fflush(stdout);
    return 0;
}

static inline int match_device(Device* dev, MtpArgs* params) {
    int device_match = 1;
    int storage_match = 1;

    if (params->device_id) {
        char* endptr = NULL;

        device_match = 0;

        if (strncmp("SN:", params->device_id, 3) == 0) {
            // serial number match
            device_match = strcmp(params->device_id+3, dev->serial) == 0;
        } else {
            uint32_t device_num = strtoul(params->device_id, &endptr, 10);
            device_match = endptr && !*endptr && device_num == dev->number;
        }
    }

    if (params->storage_id) {
        char storage_id[9] = "";
        sprintf(storage_id, "%08x", dev->storage->id);
        storage_match = strcmp(storage_id, params->storage_id) == 0;
    }

    return device_match && storage_match;
}

MtpStatusCode mtp_each_device(MtpDeviceFn callback, MtpArgs* params, void* data) {
    MtpStatusCode code = MTP_STATUS_EFAIL;
    LIBMTP_mtpdevice_t* device = NULL;
    Device* d = NULL;

    mtp_init_once();

    MtpRawDevices raw_devices = mtp_detect_raw_devices();

    if (raw_devices.status != MTP_STATUS_OK) {
        code = raw_devices.status;
        goto done;
    }

    int matched_devices = 0;
    for (int i = 0; i < raw_devices.count; i++) {
        device = mtp_open_raw_device(&raw_devices.devices[i], i);
        if (!device) {
            code = MTP_STATUS_EDEVICE;
            goto done;
        }

        for (LIBMTP_devicestorage_t* storage = device->storage; storage != 0; storage = storage->next) {
            d = device_new(i, device, storage);
            if (!d) goto done;

            if (match_device(d, params)) {
                matched_devices++;
                MtpStatusCode callback_status = callback(d, data);
                if (callback_status != MTP_STATUS_OK) {
                    code = callback_status;
                    goto done;
                }
            }

            device_free(d);
            d = NULL;
        }

        mtp_release_device(device);
        device = NULL;
    }

    code = matched_devices ? MTP_STATUS_OK : MTP_STATUS_ENODEV;

done:
    mtp_release_device(device);
    free(raw_devices.devices);
    device_free(d);
    return code;
}

MtpStatusCode mtp_mkdir(Device* dev, SyncPlan* plan) {
    MtpStatusCode code = MTP_STATUS_EFAIL;
    DeviceFile* dfile = NULL;
    char* new_path = NULL;
    char* path_dname = NULL;
    char* path_bname = NULL;

    char* path = plan->target->path;
    if (strcmp(path, "/") == 0) {
        code = MTP_STATUS_OK;
        goto done;
    }

    File* existing_file = device_get_file(dev, path);
    if (existing_file) {
        if (!existing_file->is_folder) {
            code = MTP_STATUS_EEXIST;
            goto done;
        }
        code = MTP_STATUS_OK;
        goto done;
    }

    new_path = strdup(path);
    if (!new_path) goto done;

    path_dname = fs_dirname(path);
    if (!path_dname) goto done;

    path_bname = fs_basename(path);
    if (!path_bname) goto done;

    uint32_t parent_id = 0;
    if (strcmp("/", path_dname) != 0) {
        File* parent_dir = device_get_file(dev, path_dname);
        if (!parent_dir || !parent_dir->is_folder || !parent_dir->data) goto done;
        DeviceFile* parent_df = parent_dir->data;
        parent_id = parent_df->id;
    }

    dfile = malloc(sizeof(DeviceFile));
    if (!dfile) goto done;
    dfile->is_folder = 1;
    dfile->path = new_path;
    dfile->size = 0;
    dfile->id = LIBMTP_Create_Folder(dev->device, path_bname, parent_id, dev->storage->id);

    printf("%s: %s/: ", MTP_MKDIR_MSG, path);
    if (dfile->id == 0) {
        printf("Failed!\n");
        code = MTP_STATUS_EDEVICE;
        LIBMTP_Dump_Errorstack(dev->device);
        LIBMTP_Clear_Errorstack(dev->device);
        goto done;
    }
    printf("OK\n");

    if (device_add_file(dev, dfile) != DEVICE_STATUS_OK) goto done;

    code = MTP_STATUS_OK;
    dfile = NULL;
    new_path = NULL;

done:
    free(dfile);
    free(new_path);
    free(path_bname);
    free(path_dname);
    return code;
}

MtpStatusCode mtp_get_file(Device* dev, SyncPlan* plan) {
    MtpStatusCode code = MTP_STATUS_EFAIL;
    File* f = device_get_file(dev, plan->source->path);
    char* target = plan->target->path;

    if (!f || !f->data || f->is_folder) goto done;

    DeviceFile* df = f->data;
    MtpOperationData op = { .name = MTP_PULL_MSG, .file = target };
    if (LIBMTP_Get_File_To_File(dev->device, df->id, target, mtp_progress, &op) != 0) {
        fprintf(stdout, "\n");
        fprintf(stderr, "Error getting file from MTP device.\n");
        LIBMTP_Dump_Errorstack(dev->device);
        LIBMTP_Clear_Errorstack(dev->device);
        goto done;
    }
    printf("\n");

    code = MTP_STATUS_OK;

done:
    return code;
}

MtpStatusCode mtp_send_file(Device* dev, SyncPlan* plan) {
    MtpStatusCode code = MTP_STATUS_EFAIL;
    DeviceFile* dfile = NULL;
    LIBMTP_file_t* mtp_file = NULL;
    char* new_path = NULL;
    char* dname = NULL;
    char* bname = NULL;
    char* lcname = NULL;

    if (device_get_file(dev, plan->target->path)) {
        fprintf(stderr, "File already exists: %s, skipping\n", plan->target->path);
        code = MTP_STATUS_EEXIST;
        goto done;
    }

    struct stat s;
    if (lstat(plan->source->path, &s) != 0) goto done;

    new_path = strdup(plan->target->path);
    if (!new_path) goto done;

    dname = fs_dirname(plan->target->path);
    if (!dname) goto done;

    bname = fs_basename(plan->target->path);
    if (!bname) goto done;

    lcname = str_lower(bname);
    if (!lcname) goto done;

    if (s.st_size > dev->capacity) {
        code = MTP_STATUS_ENOSPC;
        goto done;
    }

    uint32_t parent_id = 0;
    if (strcmp("/", dname) != 0) {
        File* parent_dir = device_get_file(dev, dname);
        if (!parent_dir || !parent_dir->is_folder || !parent_dir->data) goto done;
        DeviceFile* parent_df = parent_dir->data;
        parent_id = parent_df->id;
    }

    mtp_file = LIBMTP_new_file_t();
    if (!mtp_file) goto done;
    mtp_file->filesize = s.st_size;
    mtp_file->filetype = LIBMTP_FILETYPE_UNKNOWN;
    mtp_file->filename = bname;
    mtp_file->parent_id = parent_id;
    mtp_file->storage_id = dev->storage->id;
    bname = NULL;

    dfile = malloc(sizeof(DeviceFile));
    if (!dfile) goto done;
    dfile->is_folder = 0;
    dfile->size = s.st_size;
    dfile->path = new_path;
    dfile->id = 0;

    for (size_t i = 0; i < ARRAY_LEN(file_types); i++) {
        MtpPushFileType t = file_types[i];
        if (str_ends_with(lcname, t.extension)) {
            mtp_file->filetype = t.type;
            break;
        }
    }

    MtpOperationData op = { .name = MTP_PUSH_MSG, .file = plan->target->path };
    if (LIBMTP_Send_File_From_File(dev->device, plan->source->path, mtp_file, mtp_progress, &op) != 0) {
        fprintf(stdout, "\n");
        fprintf(stderr, "Error getting file from MTP device.\n");
        LIBMTP_Dump_Errorstack(dev->device);
        LIBMTP_Clear_Errorstack(dev->device);
        goto done;
    }
    printf("\n");

    dfile->id = mtp_file->item_id;

    if (device_add_file(dev, dfile) != DEVICE_STATUS_OK) goto done;
    dev->capacity -= s.st_size;

    code = MTP_STATUS_OK;
    dfile = NULL;
    new_path = NULL;

done:
    free(dfile);
    free(new_path);
    free(dname);
    free(bname);
    free(lcname);
    LIBMTP_destroy_file_t(mtp_file);
    return code;
}

MtpStatusCode mtp_rm_file(Device* dev, SyncPlan* plan) {
    MtpStatusCode code = MTP_STATUS_EFAIL;
    HashEntry* entry = NULL;

    entry = hash_remove(dev->files, plan->target->path);
    if (!entry) goto done;

    File* f = hash_entry_value(entry);
    DeviceFile* df = f->data;

    printf("%s: %s%s: ", MTP_RM_MSG, f->path, f->is_folder ? "/" : "");
    if (LIBMTP_Delete_Object(dev->device, df->id) != 0) {
        printf("Failed!\n");
        code = MTP_STATUS_EDEVICE;
        LIBMTP_Dump_Errorstack(dev->device);
        LIBMTP_Clear_Errorstack(dev->device);
        goto done;
    }
    printf("OK\n");

    code = MTP_STATUS_OK;

done:
    device_hash_entry_free(entry);
    return code;
}

static MtpStatusCode local_mkdir(SyncPlan* plan) {
    char* path = plan->target->path;
    printf("%s: %s/: ", MTP_MKDIR_MSG, path);
    if (fs_mkdir(path) != FS_STATUS_OK) {
        printf("Failed!\n");
        fprintf(stderr, "fs_mkdir(%s) failed: ", path);
        perror(NULL);
        return MTP_STATUS_EFAIL;
    }
    printf("OK\n");
    return MTP_STATUS_OK;
}

static MtpStatusCode local_rm(SyncPlan* plan) {
    char* path = plan->target->path;
    printf("%s: %s%s: ", MTP_RM_MSG, path, plan->target->is_folder ? "/" : "");
    if (fs_rm(path) != FS_STATUS_OK) {
        printf("Failed!\n");
        fprintf(stderr, "fs_rm(%s) failed: ", path);
        perror(NULL);
        return MTP_STATUS_EFAIL;
    }
    printf("OK\n");
    return MTP_STATUS_OK;
}

MtpStatusCode mtp_execute_pull_plan(Device* dev, List* plans) {
    MtpStatusCode code = MTP_STATUS_OK;

    for (size_t i = 0; i < list_size(plans); i++) {
        SyncPlan* plan = list_get(plans, i);

        switch (plan->action) {
            case SYNC_ACTION_MKDIR:
                code = local_mkdir(plan);
                break;

            case SYNC_ACTION_XFER:
                code = mtp_get_file(dev, plan);
                break;

            case SYNC_ACTION_RM:
                code = local_rm(plan);
                break;
        }

        if (code != MTP_STATUS_OK) break;
    }

    return code;
}

MtpStatusCode mtp_execute_push_plan(Device* dev, List* plans) {
    MtpStatusCode code = MTP_STATUS_OK;

    for (size_t i = 0; i < list_size(plans); i++) {
        SyncPlan* plan = list_get(plans, i);

        switch (plan->action) {
            case SYNC_ACTION_MKDIR:
                code = mtp_mkdir(dev, plan);
                break;

            case SYNC_ACTION_XFER:
                code = mtp_send_file(dev, plan);
                break;

            case SYNC_ACTION_RM:
                code = mtp_rm_file(dev, plan);
                break;
        }

        if (code != MTP_STATUS_OK) break;
    }

    return code;
}
