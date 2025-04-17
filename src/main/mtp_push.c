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

#define MTP_PULL_LIST_INIT_SIZE 512

typedef struct {
    char* from_path;
    char* to_path;
} MtpPushParams;

typedef struct {
    char* source;
    char* target;
} MtpPushSpec;

typedef struct {
    int cleanup;
    List* push_specs;
    char* to_path;
} MtpSyncSpec;

typedef struct {
    MtpStatusCode status;
    uint32_t id;
} MtpPushResult;

typedef struct {
    char* extension;
    LIBMTP_filetype_t type;
} MtpPushFileType;

MtpPushFileType FILE_TYPES[] = {
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

size_t FILE_TYPE_COUNT = sizeof(FILE_TYPES) / sizeof(MtpPushFileType);

static inline int str_sort_alpha(const void* a, const void* b) {
    const char* aa = *(const char**)a;
    const char* bb = *(const char**)b;
    return strcmp(aa, bb);
}

static void free_push_spec(void* data) {
    if (data) {
        MtpPushSpec* push_spec = (MtpPushSpec*)data;
        free(push_spec->target);
        free(push_spec);
    }
}

static MtpPushResult mtp_mkdir_try(Device* dev, char* path) {
    MtpPushResult result = {
        .status = MTP_STATUS_EFAIL,
        .id = 0
    };

    DeviceFile* file = NULL;
    char* new_path = NULL;
    char* path_dname = NULL;
    char* path_bname = NULL;

    Hash* file_hash = dev->files;
    if (strcmp(path, "/") == 0) {
        result.status = MTP_STATUS_OK;
        goto done;
    }

    DeviceFile* existing_file = hash_get(file_hash, path);
    if (existing_file) {
        if (!existing_file->is_folder) {
            result.status = MTP_STATUS_EEXIST;
            goto done;
        }
        result.status = MTP_STATUS_OK;
        result.id = existing_file->id;
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
        DeviceFile* parent_dir = hash_get(file_hash, path_dname);
        if (!parent_dir || !parent_dir->is_folder) goto done;
        parent_id = parent_dir->id;
    }

    file = malloc(sizeof(DeviceFile));
    if (!file) goto done;
    file->is_folder = 1;
    file->path = new_path;
    file->size = 0;
    file->id = LIBMTP_Create_Folder(dev->device, path_bname, parent_id, dev->storage->id);

    printf("%s: %s/: ", MTP_MKDIR_MSG, path);
    if (file->id == 0) {
        printf("Failed!\n");
        result.status = MTP_STATUS_EDEVICE;
        LIBMTP_Dump_Errorstack(dev->device);
        LIBMTP_Clear_Errorstack(dev->device);
        goto done;
    }
    printf("OK\n");

    if (device_add_file(dev, file) != DEVICE_STATUS_OK) goto done;

    result.status = MTP_STATUS_OK;
    result.id = file->id;

    file = NULL;
    new_path = NULL;

done:
    free(file);
    free(new_path);
    free(path_bname);
    free(path_dname);
    return result;
}

static MtpPushResult mtp_mkdirp(Device* dev, char* path) {
    MtpPushResult result = {
        .status = MTP_STATUS_EFAIL,
        .id = 0,
    };

    char* tmp = fs_resolve_cwd("/", path);
    if (!tmp) goto done;

    for (char* p = tmp+1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            MtpStatusCode status = mtp_mkdir_try(dev, tmp).status;
            if (status != MTP_STATUS_OK) goto done;
            *p = '/';
        }
    }

    result = mtp_mkdir_try(dev, tmp);

done:
    free(tmp);
    return result;
}

static MtpPushResult mtp_ensure_dir(Device* dev, MtpPushSpec* spec) {
    MtpPushResult result = {
        .status = MTP_STATUS_EFAIL,
        .id = 0,
    };

    char* dname = NULL;

    dname = fs_dirname(spec->target);
    if (!dname) goto done;

    result = mtp_mkdirp(dev, dname);

done:
    free(dname);
    return result;
}

static MtpPushResult mtp_send_file(Device* dev, uint32_t parent_id, MtpPushSpec* spec) {
    MtpPushResult result = {
        .status = MTP_STATUS_EFAIL,
        .id = 0,
    };

    DeviceFile* file = NULL;
    LIBMTP_file_t* mtp_file = NULL;
    char* new_path = NULL;
    char* bname = NULL;
    char* lcname = NULL;

    Hash* file_hash = dev->files;
    DeviceFile* existing_file = hash_get(file_hash, spec->target);
    if (existing_file) {
        fprintf(stderr, "File already exists: %s (%u), skipping\n", spec->target, existing_file->id);
        result.status = MTP_STATUS_EEXIST;
        goto done;
    }

    struct stat s;
    if (lstat(spec->source, &s) != 0) goto done;

    new_path = strdup(spec->target);
    if (!new_path) goto done;

    bname = fs_basename(spec->target);
    if (!bname) goto done;

    lcname = str_lower(bname);
    if (!lcname) goto done;

    if (s.st_size > dev->capacity) {
        result.status = MTP_STATUS_ENOSPC;
        goto done;
    }

    mtp_file = LIBMTP_new_file_t();
    if (!mtp_file) goto done;
    mtp_file->filesize = s.st_size;
    mtp_file->filetype = LIBMTP_FILETYPE_UNKNOWN;
    mtp_file->filename = bname;
    mtp_file->parent_id = parent_id;
    mtp_file->storage_id = dev->storage->id;
    bname = NULL;

    file = malloc(sizeof(DeviceFile));
    if (!file) goto done;
    file->is_folder = 0;
    file->size = s.st_size;
    file->path = new_path;
    file->id = 0;

    for (size_t i = 0; i < FILE_TYPE_COUNT; i++) {
        MtpPushFileType t = FILE_TYPES[i];
        if (str_ends_with(lcname, t.extension)) {
            mtp_file->filetype = t.type;
            break;
        }
    }

    MtpOperationData op = { .name = MTP_PUSH_MSG, .file = spec->target };
    if (LIBMTP_Send_File_From_File(dev->device, spec->source, mtp_file, mtp_progress, &op) != 0) {
        fprintf(stdout, "\n");
        fprintf(stderr, "Error getting file from MTP device.\n");
        LIBMTP_Dump_Errorstack(dev->device);
        LIBMTP_Clear_Errorstack(dev->device);
        goto done;
    }
    printf("\n");

    file->id = mtp_file->item_id;

    if (device_add_file(dev, file) != DEVICE_STATUS_OK) goto done;
    dev->capacity -= s.st_size;

    result.status = MTP_STATUS_OK;
    result.id = file->id;

    file = NULL;
    new_path = NULL;

done:
    free(file);
    free(new_path);
    free(bname);
    free(lcname);
    LIBMTP_destroy_file_t(mtp_file);
    return result;
}

static MtpStatusCode mtp_cleanup_files(Device* dev, MtpSyncSpec* sync_spec, List* push_specs) {
    MtpStatusCode code = MTP_STATUS_EFAIL;
    List* files_after = NULL;
    List* files_stray = NULL;
    Hash* target_hash = NULL;
    char* target = NULL;

    files_after = device_filter_files(dev, sync_spec->to_path);
    if (!files_after) goto done;

    files_stray = list_new(list_size(files_after));
    if (!files_after) goto done;

    target_hash = hash_new_str(list_size(push_specs) * 2);
    if (!target_hash) goto done;

    for (size_t i = 0; i < list_size(push_specs); i++) {
        MtpPushSpec* spec = list_get(push_specs, i);

        target = strdup(spec->target);
        if (!target) goto done;

        do {
            HashPutResult r = hash_put(target_hash, target, spec);
            int is_existing = r.old_entry != NULL;
            free(hash_entry_key(r.old_entry));
            hash_entry_free(r.old_entry);
            if (r.status != HASH_STATUS_OK) goto done;

            if (is_existing) break;

            target = fs_dirname(target);
            if (!target) goto done;
        } while (strcmp(target, "/") != 0);

        target = NULL;
    }

    for (size_t i = 0; i < list_size(files_after); i++) {
        DeviceFile* f = list_get(files_after, i);
        if (!hash_get(target_hash, f->path)) {
            if (list_push(files_stray, f) != LIST_STATUS_OK) goto done;
        }
    }

    mtp_rm_files(dev, files_stray);

    code = MTP_STATUS_OK;

done:
    free(target);
    list_free(files_stray);
    list_free(files_after);
    hash_free(target_hash);
    return code;
}

static MtpStatusCode mtp_push_callback(Device* dev, void* data) {
    MtpStatusCode code = MTP_STATUS_EFAIL;
    List* push_specs = NULL;
    char* dname = NULL;

    if (device_load(dev) != DEVICE_STATUS_OK) {
        code = MTP_STATUS_EDEVICE;
        fprintf(stderr, "Failed to load device\n");
        goto done;
    }

    Hash* file_hash = dev->files;
    MtpSyncSpec* sync_spec = (MtpSyncSpec*)data;
    List* push_specs_pre = sync_spec->push_specs;

    push_specs = list_new(list_size(push_specs_pre));
    if (!push_specs) goto done;

    for (size_t i = 0; i < list_size(push_specs_pre); i++) {
        MtpPushSpec* push_spec = list_get(push_specs_pre, i);
        DeviceFile* f = hash_get(file_hash, push_spec->target);
        if (f) {
            printf("%s: %s\n", MTP_SKIP_MSG, push_spec->target);
        } else {
            if (list_push(push_specs, push_spec) != LIST_STATUS_OK) goto done;
            printf("%s: %s\n", MTP_PUSH_MSG, push_spec->target);
        }
    }

    if (list_size(push_specs)) {
        if (!io_confirm("Proceed [y/n]? ")) {
            code = MTP_STATUS_EREJECT;
            goto done;
        }

        for (size_t i = 0; i < list_size(push_specs); i++) {
            MtpPushSpec* push_spec = list_get(push_specs, i);
            MtpPushResult dir_result = mtp_ensure_dir(dev, push_spec);
            if (dir_result.status != MTP_STATUS_OK) goto done;

            MtpPushResult file_result = mtp_send_file(dev, dir_result.id, push_spec);
            if (file_result.status != MTP_STATUS_OK && file_result.status != MTP_STATUS_EEXIST) goto done;
        }
    } else {
        printf("All files already present on the device.\n");
        code = MTP_STATUS_OK;
    }

    if (sync_spec->cleanup) {
        if (mtp_cleanup_files(dev, sync_spec, push_specs_pre) != MTP_STATUS_OK) goto done;
    }

    code = MTP_STATUS_OK;

done:
    free(dname);
    list_free(push_specs);
    return code;
}

MtpStatusCode mtp_push(MtpDeviceParams* mtp_params, char* from_path, char* to_path, int cleanup) {
    MtpStatusCode code = MTP_STATUS_EFAIL;
    MtpPushSpec* push_spec = NULL;
    List* push_specs = NULL;
    List* files = NULL;
    List* files_sorted = NULL;
    char* from_path_r = NULL;
    char* to_path_r = NULL;
    char* target = NULL;

    push_specs = list_new(MTP_PULL_LIST_INIT_SIZE);
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
        char* target = NULL;
        MtpPushSpec* push_spec = NULL;

        char* source = list_get(files_sorted, i);
        target = fs_path_join(to_path_r, source + from_path_len);
        if (!target) goto done;

        push_spec = malloc(sizeof(MtpPushSpec));
        if (!push_spec) goto done;
        push_spec->source = source;
        push_spec->target = target;

        if (list_push(push_specs, push_spec) != LIST_STATUS_OK) goto done;

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
    free(push_spec);
    free(target);
    free(from_path_r);
    free(to_path_r);
    list_free(files_sorted);
    list_free_deep(files, free);
    list_free_deep(push_specs, free_push_spec);
    return code;
}
