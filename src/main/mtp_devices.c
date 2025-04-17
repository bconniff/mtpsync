#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>

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
        printf("\33[1mDevice:\33[0m %s\n", name);
        printf(" * \33[1mNumber\33[0m: %i\n", d->number);
        printf(" * \33[1mSerial\33[0m: SN:%s\n", d->serial);
    }

    int free_percent = (d->storage->FreeSpaceInBytes*100)/(d->storage->MaxCapacity);

    // print storage info
    printf(" * \33[1mStorage\33[0m: %s\n", d->storage->StorageDescription);
    printf("   - \33[1mID\33[0m: %08x\n", d->storage->id);
    printf("   - \33[1mFree Space\33[0m: %d%% (%lu bytes)\n", free_percent, d->storage->FreeSpaceInBytes);

    code = MTP_STATUS_OK;

done:
    free(name);
    return code;
}

MtpStatusCode mtp_devices(MtpDeviceParams* mtp_params) {
    int current_device_number = -1;
    return mtp_each_device(mtp_devices_callback, mtp_params, &current_device_number);
}
