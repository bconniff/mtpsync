/**
 * @file device.h
 * Wrapper to encapsulate a specific MTP device and storage volume.
 */

#ifndef _DEVICE_H_
#define _DEVICE_H_

#include <libmtp.h>

#include "hash.h"

/**
 * Status codes for device operations
 */
typedef enum {
    DEVICE_STATUS_OK,     ///< Operation successful
    DEVICE_STATUS_EFAIL,  ///< Failed due to a general runtime error
} DeviceStatusCode;

/**
 * Container for a specific MTP device and storage volume.
 */
typedef struct {
    int number;                       ///< Index of the device
    char* serial;                     ///< Serial number
    Hash* files;                      ///< Hash of all files on the device
    uint64_t capacity;                ///< Remaining storage capacity in bytes
    LIBMTP_mtpdevice_t* device;       ///< Raw MTP device
    LIBMTP_devicestorage_t* storage;  ///< Raw MTP storage volume
} Device;

/**
 * Represents a folder or file on the MTP device.
 */
typedef struct {
    uint32_t id;    ///< Unique ID of the file
    uint64_t size;  ///< Size of the file in bytes
    int is_folder;  ///< Truthy if this represents a folder
    char* path;     ///< Full, canonical path of the file
} DeviceFile;

/**
 * Create a new device container for a specific MTP device and storage
 * combination. The files hash will be NULL until device_load is called.
 * Free it with device_free when done.
 * @param number   index of the device
 * @param device   raw MTP device
 * @param storage  storage volume
 * @return         new device, or NULL in case of failure
 */
Device* device_new(int number, LIBMTP_mtpdevice_t* device, LIBMTP_devicestorage_t* storage);

/**
 * Loads files from the device in to the files hash. This can be slow.
 * @param d  device to load files for
 * @return   status code of the operation
 */
DeviceStatusCode device_load(Device* d);

/**
 * Returns all files within the specified path. Call device_load first.
 * @param d     device to load files for
 * @param path  path to search for files in
 * @return      list of all matching file, or NULL in case of failure
 */
List* device_filter_files(Device* d, char* path);

/**
 * Adds a new file to the device's files hash. Call device_load first. Note
 * that this does not actually transfer any data to the attached MTP device,
 * and is only use to cache the new file in the files hash. Clients should
 * handle MTP operations on their own.
 * @param d  device to add the file to
 * @param f  path to search for files in
 * @return   list of all matching file, or NULL in case of failure
 */
DeviceStatusCode device_add_file(Device* d, DeviceFile* f);

/**
 * Frees the device and any associated data, including the files hash.
 * @param d  device to free
 */
void device_free(Device* d);

/**
 * Free a device file.
 * @param df  device file to free
 */
void device_file_free(DeviceFile* df);

/**
 * Free a hash entry from the device file hash.
 * @param e  device file hash entry to free
 */
void device_hash_entry_free(HashEntry* e);

#endif
