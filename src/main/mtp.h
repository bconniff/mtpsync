#ifndef _MTP_H_
#define _MTP_H_

#include "list.h"
#include "device.h"
#include "color.h"

#define MTP_SKIP_MSG   C_BOLD C_YELLOW "SKIP" C_RESET
#define MTP_PULL_MSG   C_BOLD C_CYAN "PULL" C_RESET
#define MTP_PUSH_MSG   C_BOLD C_GREEN "PUSH" C_RESET
#define MTP_RM_MSG     C_BOLD C_RED "RM" C_RESET
#define MTP_MKDIR_MSG  C_BOLD C_BLUE "MKDIR" C_RESET

typedef enum {
    MTP_STATUS_OK,      // everything ok
    MTP_STATUS_EFAIL,   // failed due to a general error
    MTP_STATUS_ENOCMD,  // invalid command specified
    MTP_STATUS_ENOIMPL, // failed due to missing implementation
    MTP_STATUS_ENOSPC,  // insufficient space on device
    MTP_STATUS_ENOMEM,  // allocation failed due to insufficient heap space
    MTP_STATUS_EDEVICE, // device error
    MTP_STATUS_EEXIST,  // file already exists
    MTP_STATUS_EREJECT, // user reject the operation interactively
    MTP_STATUS_ESYNTAX, // invoked with invalid syntax
} MtpStatusCode;

typedef struct {
    char* name;
    char* file;
} MtpOperationData;

typedef struct {
    char* device_id;
    char* storage_id;
} MtpDeviceParams;

typedef MtpStatusCode (*MtpDeviceFn)(Device* dev, void* data);

MtpStatusCode mtp_each_device(MtpDeviceFn callback, MtpDeviceParams* params, void* data);
int mtp_progress(const uint64_t sent, const uint64_t total, void const * const data);

#endif
