#include <libmtp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <libmtp.h>

#include "hash.h"
#include "device.h"
#include "fs.h"
#include "str.h"
#include "sync.h"

#define DEVICE_HASH_INIT_SIZE 512

static inline int file_sort_alpha(const void* a, const void* b) {
    const File* aa = *(const File**)a;
    const File* bb = *(const File**)b;
    int result = strcmp(aa->path, bb->path);
    return result;
}

void device_file_free(DeviceFile* f) {
    if (f) {
        free(f->path);
        free(f);
    }
}

void device_hash_entry_free(HashEntry* e) {
    if (e) {
        File* f = hash_entry_value(e);
        if (f) {
            device_file_free(f->data);
            file_free(f);
        }
        hash_entry_free(e);
    }
}

static void free_files(LIBMTP_file_t* files) {
    for (LIBMTP_file_t* f = files; f != NULL; ) {
        LIBMTP_file_t* prev = f;
        f = f->next;
        LIBMTP_destroy_file_t(prev);
    }
}

static DeviceStatusCode device_load_files_recursive(
    Device* d,
    DeviceFile* parent,
    uint32_t file_id
);

static DeviceStatusCode device_load_files_recursive(
    Device* d,
    DeviceFile* parent,
    uint32_t file_id
) {
    int code = DEVICE_STATUS_EFAIL;
    LIBMTP_file_t* files = NULL;
    char* path = NULL;
    DeviceFile* device_file = NULL;

    LIBMTP_mtpdevice_t* device = d->device;
    LIBMTP_devicestorage_t* storage = d->storage;

    files = LIBMTP_Get_Files_And_Folders(device, storage->id, file_id);
    if (LIBMTP_Get_Errorstack(device)) {
        LIBMTP_Dump_Errorstack(device);
        LIBMTP_Clear_Errorstack(device);
        goto done;
    }

    if (!files) {
        code = DEVICE_STATUS_OK;
        goto done;
    }

    for (LIBMTP_file_t* file = files; file; file = file->next) {
        char* parent_path = parent ? parent->path : "/";

        path = fs_path_join(parent_path, file->filename);
        if (!path) goto done;

        printf("\33[2K\r\33[1mFILE\33[0m: %u: %s", file->item_id, path);
        fflush(stdout);

        device_file = malloc(sizeof(DeviceFile));
        if (!device_file) goto done;

        device_file->id = file->item_id;
        device_file->size = file->filesize;
        device_file->path = path;
        device_file->is_folder = (file->filetype == LIBMTP_FILETYPE_FOLDER);

        if (device_file->is_folder) {
            if (device_load_files_recursive(d, device_file, file->item_id) != DEVICE_STATUS_OK) goto done;
        }

        if (device_add_file(d, device_file) != DEVICE_STATUS_OK) goto done;

        path = NULL;
        device_file = NULL;
    }

    code = DEVICE_STATUS_OK;

done:
    free(path);
    free(device_file);
    free_files(files);
    return code;
}

File* device_get_file(Device* d, char* path) {
    return hash_get(d->files, path);
}

DeviceStatusCode device_add_file(Device* d, DeviceFile* dfile) {
    DeviceStatusCode code = DEVICE_STATUS_EFAIL;
    File* file = NULL;

    file = file_new_data(dfile->path, dfile->is_folder, dfile);
    if (!file) goto done;

    HashPutResult r = hash_put(d->files, file->path, file);
    device_hash_entry_free(r.old_entry);
    if (r.status != HASH_STATUS_OK) goto done;

    code = DEVICE_STATUS_OK;
    file = NULL;

done:
    file_free(file);
    return code;
}

Device* device_new(int number, LIBMTP_mtpdevice_t* device, LIBMTP_devicestorage_t* storage) {
    Device* d = NULL;
    char* serial = NULL;

    d = malloc(sizeof(Device));
    if (!d) goto error;

    serial = LIBMTP_Get_Serialnumber(device);
    if (!serial) goto error;

    d->number = number;
    d->capacity = storage->FreeSpaceInBytes;
    d->device = device;
    d->storage = storage;
    d->files = NULL;
    d->serial = serial;

    return d;

error:
    free(d);
    free(serial);
    return NULL;
}

DeviceStatusCode device_load(Device* d) {
    Hash* new_files = NULL;

    hash_free_deep(d->files, device_hash_entry_free);
    d->files = NULL;

    new_files = hash_new_str(DEVICE_HASH_INIT_SIZE);
    if (!new_files) goto error;
    d->files = new_files;

    if (device_load_files_recursive(d, NULL, LIBMTP_FILES_AND_FOLDERS_ROOT) != 0) {
        printf("\33[2K\rFailed!\n");
        goto error;
    }
    printf("\33[2K\rDone, received %zu files.\n", hash_size(new_files));

    return DEVICE_STATUS_OK;

error:
    hash_free_deep(new_files, device_hash_entry_free);
    d->files = NULL;
    return DEVICE_STATUS_EFAIL;
}

void device_free(Device* d) {
    if (d) {
        free(d->serial);
        hash_free_deep(d->files, device_hash_entry_free);
    }
    free(d);
}

static int is_within_path(void* item, void* data) {
    File* f = item;
    char* path = data;
    size_t i = 0;

    if (strcmp("/", path) == 0) return 1;

    // check that the file_path begins with the path
    char* file_path = f->path;
    for (i = 0; file_path[i] && path[i] && file_path[i] == path[i]; i++);
    if (path[i]) return 0;

    // check that next char in file_path is a null byte or path separator
    return file_path[i] == '/' || !file_path[i];
}

List* device_filter_files(Device* dev, char* path) {
    List* files = NULL;
    List* filtered = NULL;
    List* sorted = NULL;

    files = hash_values(dev->files);
    if (!files) goto done;

    filtered = list_filter_data(files, is_within_path, path);
    if (!filtered) goto done;

    sorted = list_sort(filtered, file_sort_alpha);

done:
    list_free(files);
    list_free(filtered);
    return sorted;
}
