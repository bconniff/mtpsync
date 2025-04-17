#ifndef _MTP_H_
#define _MTP_H_

#include "device.h"
#include "color.h"

#define MTP_SKIP_MSG   C_BOLD C_YELLOW "SKIP" C_RESET
#define MTP_PULL_MSG   C_BOLD C_CYAN "PULL" C_RESET
#define MTP_PUSH_MSG   C_BOLD C_GREEN "PUSH" C_RESET
#define MTP_RM_MSG     C_BOLD C_RED "RM" C_RESET
#define MTP_MKDIR_MSG  C_BOLD C_BLUE "MKDIR" C_RESET

/**
 * Status codes for various MTP operations.
 */
typedef enum {
    MTP_STATUS_OK,       ///< Operation succeeded
    MTP_STATUS_EFAIL,    ///< Failed due to a general error
    MTP_STATUS_ENOCMD,   ///< Invalid command specified
    MTP_STATUS_ENOIMPL,  ///< Failed due to missing implementation
    MTP_STATUS_ENOSPC,   ///< Insufficient space on device
    MTP_STATUS_ENOMEM,   ///< Allocation failed due to insufficient heap space
    MTP_STATUS_EDEVICE,  ///< Device error
    MTP_STATUS_EEXIST,   ///< File already exists
    MTP_STATUS_EREJECT,  ///< User rejected the operation interactively
    MTP_STATUS_ESYNTAX,  ///< Invoked with invalid syntax
} MtpStatusCode;

/**
 * Data struct provided to the mtp_progress callback function.
 */
typedef struct {
    char* name;  ///< Name of the MTP operation
    char* file;  ///< Name of the file being processed
} MtpOperationData;

/**
 * Device filtering parameters to use with mtp_each_device.
 */
typedef struct {
    char* device_id;   ///< Device index, serial number, or NULL for all
    char* storage_id;  ///< Storage ID, or NULL for all
} MtpDeviceParams;

/**
 * Callback function for mtp_each_device.
 * @param dev   the device to operate on
 * @param data  additional context data passed to the callback
 * @return      status code, return anything other than MTP_STATUS_OK to exit
 */
typedef MtpStatusCode (*MtpDeviceFn)(Device* dev, void* data);

/**
 * Execute a callback for each connected MTP device and storage combination.
 * This function handles scanning devices and storages, opening, and releasing
 * the device when done. An open device and storage handle will be passed to
 * the callback along with the additional data parameter.
 * @param callback  to invoke for each device & storage combination
 * @param params    used to select specific devies or storage by ID
 * @param data      additional context data provided to the callback function
 * @return          status code indicating success or failure
 */
MtpStatusCode mtp_each_device(MtpDeviceFn callback, MtpDeviceParams* params, void* data);

/**
 * Display progress for any operations that send files to or from the device.
 * @param sent   number of bytes sent/received
 * @param total  total number of bytes
 * @param data   pointer to MtpOperationData representing the operation
 */
int mtp_progress(const uint64_t sent, const uint64_t total, void const * const data);

#endif
