#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>

#include "color.h"
#include "device.h"
#include "mtp.h"

static MtpStatusCode mtp_devices_callback(Device* d, void* data) {
    MtpStatusCode code = MTP_STATUS_EFAIL;
    char* name = NULL;

    int* current_device_number = (int*)data;

    // print header for device each time a new device is found
    if (*current_device_number != d->number) {
        *current_device_number = d->number;

        name = LIBMTP_Get_Friendlyname(d->device);
        if (!name) name = strdup("Unknown");
        if (!name) goto done;

        printf("\n");
        printf(C_BOLD "Device:" C_RESET " %s\n", name);
        printf(" * " C_BOLD "Number:" C_RESET " %i\n", d->number);
        printf(" * " C_BOLD "Serial:" C_RESET " SN:%s\n", d->serial);
    }

    int free_percent = (d->storage->FreeSpaceInBytes*100)/(d->storage->MaxCapacity);

    // print storage info
    printf(" * " C_BOLD "Storage:" C_RESET " %s\n", d->storage->StorageDescription);
    printf("   - " C_BOLD "ID:" C_RESET " %08x\n", d->storage->id);
    printf("   - " C_BOLD "Free Space:" C_RESET " %d%% (%llu bytes)\n", free_percent, (long long unsigned int)d->storage->FreeSpaceInBytes);

    code = MTP_STATUS_OK;

done:
    free(name);
    return code;
}

MtpStatusCode mtp_devices(MtpDeviceParams* mtp_params) {
    int current_device_number = -1;
    return mtp_each_device(mtp_devices_callback, mtp_params, &current_device_number);
}
