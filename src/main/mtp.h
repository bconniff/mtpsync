/**
 * @file mtp.h
 * Common functions for interacting with the MTP protocol.
 */

#ifndef _MTP_H_
#define _MTP_H_

#include "device.h"
#include "color.h"
#include "sync.h"

#define MTP_SKIP_MSG   C_BOLD C_YELLOW "SKIP" C_RESET ///< skipped message
#define MTP_PULL_MSG   C_BOLD C_CYAN "PULL" C_RESET   ///< pull file to local
#define MTP_PUSH_MSG   C_BOLD C_GREEN "PUSH" C_RESET  ///< push file to device
#define MTP_RM_MSG     C_BOLD C_RED "RM" C_RESET      ///< remove file message
#define MTP_MKDIR_MSG  C_BOLD C_BLUE "MKDIR" C_RESET  ///< mkdir message

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
 * Program command-line arguments.
 */
typedef struct {
    char* device_id;  ///< Device index, serial number, or NULL for all
    char* storage_id; ///< Storage ID, or NULL for all
    int yes;          ///< If truthy, skip interaction and assume "yes"
    int cleanup;      ///< If truthy, remove stray files after push/pull
} MtpArgs;

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
 * @param args      used to select specific devies or storage by ID
 * @param data      additional context data provided to the callback function
 * @return          status code indicating success or failure
 */
MtpStatusCode mtp_each_device(MtpDeviceFn callback, MtpArgs* args, void* data);

/**
 * Send a local file to an MTP device.
 * @param dev   device to operate on
 * @param plan  plan for file to send
 * @return      status code
 */
MtpStatusCode mtp_send_file(Device* dev, SyncPlan* plan);

/**
 * Retrieve a file from an MTP device to local system.
 * @param dev   device to operate on
 * @param plan  plan for file to get
 * @return      status code
 */
MtpStatusCode mtp_get_file(Device* dev, SyncPlan* plan);

/**
 * Delete a file or directory from an MTP device.
 * @param dev   device to operate on
 * @param plan  plan for file to delete
 * @return      status code
 */
MtpStatusCode mtp_rm_file(Device* dev, SyncPlan* plan);

/**
 * Create a directory on an MTP device.
 * @param dev   device to operate on
 * @param plan  plan for creating the directory
 * @return      status code
 */
MtpStatusCode mtp_mkdir(Device* dev, SyncPlan* plan);

/**
 * Execute plan to push files to a device.
 * @param dev   device to operate on
 * @param plan  list of plans to execute
 * @return      status code
 */
MtpStatusCode mtp_execute_push_plan(Device* dev, List* plan);

/**
 * Execute plan to pull files from a device.
 * @param dev   device to operate on
 * @param plan  list of plans to execute
 * @return      status code
 */
MtpStatusCode mtp_execute_pull_plan(Device* dev, List* plan);

#endif
