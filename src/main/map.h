/**
 * @file map.h
 * Implements map functions for converting between different types.
 */

#ifndef _MAP_H_
#define _MAP_H_

/**
 * Status of map callback function.
 */
typedef enum {
    MAP_STATUS_OK,    ///< Data mapped successfully
    MAP_STATUS_EFAIL, ///< Error occurred mapping data
} MapStatusCode;

/**
 * Convert a DeviceFile to a SyncFile.
 * @param df    DeviceFile to convert
 * @param code  output parameter pointer to a MapStatusCode
 * @return      newly created SyncFile
 */
SyncFile* map_device_file_to_sync_file(DeviceFile* df, MapStatusCode* code);

/**
 * Convert a SyncSpec to a SyncFile.
 * @param spec  SyncSpec to convert
 * @param code  output parameter pointer to a MapStatusCode
 * @return      newly created SyncFile
 */
SyncFile* map_sync_spec_to_sync_file(SyncSpec* df, MapStatusCode* code);

#endif
