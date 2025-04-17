#ifndef _DEVICE_H_
#define _DEVICE_H_

#include <libmtp.h>

#include "hash.h"

typedef enum {
    DEVICE_STATUS_OK,
    DEVICE_STATUS_EFAIL,
    DEVICE_STATUS_EEXIST
} DeviceStatusCode;

typedef struct {
    int number;
    char* serial;
    Hash* files;
    uint64_t capacity;
    LIBMTP_mtpdevice_t* device;
    LIBMTP_devicestorage_t* storage;
} Device;

typedef struct {
    uint32_t id;
    uint64_t size;
    int is_folder;
    char* path;
} DeviceFile;

Device* device_new(int number, LIBMTP_mtpdevice_t* device, LIBMTP_devicestorage_t* storage);
DeviceStatusCode device_load(Device* d);
List* device_filter_files(Device* d, char* path);
DeviceStatusCode device_add_file(Device* d, DeviceFile* f);
void device_free(Device* d);

#endif
