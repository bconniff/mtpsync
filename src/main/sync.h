/**
 * @file sync.h
 * Provides the main algorithm to building plans to synchronize files between
 * different storage devices. This does not provide any mechanism for talking
 * to the local file system or MTP devices and only controls the logic for
 * deciding how to perform synchronizaiton.
 */

#ifndef _SYNC_H_
#define _SYNC_H_

#include "file.h"
#include "list.h"

/**
 * File-specific action types for a sync plan. Order of the enum is important
 * since it determines which order actions are executed.
 */
typedef enum {
    SYNC_ACTION_RM,    ///< Delete a file or directory
    SYNC_ACTION_MKDIR, ///< Create a new directory
    SYNC_ACTION_XFER,  ///< Transfer the file from source to target
} SyncAction;

/**
 * A request to synchronize a file from a source to a target.
 */
typedef struct {
    char* source; ///< Path of the source file to send
    char* target; ///< Path of the target file
} SyncSpec;

/**
 * An inidivual action that represents part of a larger plan to synchronize
 * files between two systems.
 */
typedef struct {
    File* source;  ///< The source file (may be NULL for some actions)
    File* target;  ///< The target file
    SyncAction action; ///< The action to perform
} SyncPlan;

/**
 * Free a SyncPlan.
 * @param plan  plan to free
 */
void sync_plan_free(SyncPlan* plan);

/**
 * Create a new SyncPlan. Returns NULL in case of failure. Free it when done.
 * @param source  source file, will be copied
 * @param target  target file, will be copied
 * @param action  action to take
 * @return        a new SyncPlan, or NULL in case of an error
 */
SyncPlan* sync_plan_new(File* source, File* target, SyncAction action);

/**
 * Free a SyncSpec.
 * @param spec  spec to free
 */
void sync_spec_free(SyncSpec* spec);

/**
 * Create a new SyncSpec. Returns NULL in case of failure. Free it when done.
 * @param source  source file path, will be copied
 * @param target  target file path, will be copied
 * @return        a new SyncSpec, or NULL in case of an error
 */
SyncSpec* sync_spec_new(char* source, char* target);

/**
 * Create a plan for removing files.
 * @param rm_files  list of file paths to remove, must be canonicalized
 * @return          plans to remove the items, or NULL in case of error
 */
List* sync_plan_rm(List* rm_files);

/**
 * Create a plan for pushing files from source to target device. This will
 * generally do the following for each item in the specs list:
 *  - If file exists in the target_files, it will be skipped.
 *  - If file does not exist in the target_files, parent directories will be
 *    created as needed and the file will be pushed to the target.
 *  - If the cleanup option is enabled, any files present in the target_files
 *    that do not have a matching spec in the specs list will be removed
 * @param source_files  current files on the source device
 * @param target_files  current files on the target device
 * @param specs         specifications for source-to-target file mapping
 * @param cleanup       if truthy, stray files on the target will be deleted
 * @return              plans to sync files, or NULL in case of error
 */
List* sync_plan_push(List* source_files, List* target_files, List* specs, int cleanup);

/**
 * Print a sync plan to stdout for the user to review.
 * @param plan      to print
 * @param xfer_msg  message to print for file transfers
 */
void sync_plan_print(List* plan, char* xfer_msg);

/**
 * Create a list of sync specs for provided files.
 * @param files      files to create specs for
 * @param from_path  path to transfer from
 * @param to_path    path to transfer to
 * @return           list of sync specs, or NULL in case of error
 */
List* sync_spec_create(List* files, char* from_path, char* to_path);

#endif
