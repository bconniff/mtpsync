#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libmtp.h>

#include "device.h"
#include "mtp.h"
#include "str.h"
#include "list.h"

typedef struct {
    MtpStatusCode status;
    LIBMTP_raw_device_t* devices;
    int count;
} MtpRawDevices;

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
            fprintf(stdout, "No device found\n");
            result.status = MTP_STATUS_OK;
            break;

        case LIBMTP_ERROR_NONE:
            result.status = MTP_STATUS_OK;
            break;

        case LIBMTP_ERROR_CONNECTING:
            fprintf(stderr, "Could not connect to device\n");
            result.status = MTP_STATUS_EDEVICE;
            break;

        case LIBMTP_ERROR_MEMORY_ALLOCATION:
            fprintf(stderr, "Failed to allocate memory\n");
            result.status = MTP_STATUS_ENOMEM;
            break;

        default:
            fprintf(stderr, "An unknown error occurred\n");
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

static inline int match_device(Device* dev, MtpDeviceParams* params) {
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

MtpStatusCode mtp_each_device(MtpDeviceFn callback, MtpDeviceParams* params, void* data) {
    MtpStatusCode code = MTP_STATUS_EFAIL;
    LIBMTP_mtpdevice_t* device = NULL;
    Device* d = NULL;

    mtp_init_once();

    MtpRawDevices raw_devices = mtp_detect_raw_devices();

    if (raw_devices.status != MTP_STATUS_OK) {
        code = raw_devices.status;
        goto done;
    }

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

    code = MTP_STATUS_OK;

done:
    mtp_release_device(device);
    free(raw_devices.devices);
    device_free(d);
    return code;
}

int mtp_progress(const uint64_t sent, const uint64_t total, void const * const data) {
    MtpOperationData* op = (MtpOperationData*)data;
    int percent = (sent*100)/total;
    printf("\33[2K\r%s: %s: %d%%", op->name, op->file, percent);
    fflush(stdout);
    return 0;
}
