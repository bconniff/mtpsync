/**
 * @file fs.h
 * Reusable functions for working with the local filesystem.
 */

#ifndef _FS_H_
#define _FS_H_

#include "list.h"

/**
 * Status codes for file operations.
 */
typedef enum {
    FS_STATUS_OK,      ///< Operation successful
    FS_STATUS_ENOMEM,  ///< Failed due to allocation error
    FS_STATUS_EFAIL,   ///< Failed due to other unspecified runtime issue
    FS_STATUS_ENOENT,  ///< Failed because file does not exist
} FsStatusCode;

/**
 * Collect all regular files within a specified path into a list.
 * @param path  to collect files from
 * @return      list of all files under the provided path
 */
List* fs_collect_files(char* path);

/**
 * Collect all existing parent directories for a given path into a list.
 * @param path  to collect ancestors of
 * @return      list of all ancestor directories
 */
List* fs_collect_ancestors(char* path);

/**
 * Remove a file or directory
 * @param path  path to remove
 * @return      status of operation
 */
FsStatusCode fs_rm(char* path);

/**
 * Create directory path if it dose not exist.
 * @param path  directory path to create
 * @return      status of operation
 */
FsStatusCode fs_mkdir(char* path);

/**
 * Create directory path, including all non-existing parent directories.
 * Does not fail if the directory already exists.
 * @param path  directory path to create
 * @return      status of operation
 */
FsStatusCode fs_mkdirp(char* path);

/**
 * Resolve a path to a canonicalized form. This includes the following:
 *  - Convert to absolute path relative to the current working directory
 *  - Multiple sequential slashes are replaced by a single slash
 *  - Any "." segments in the path are removed
 *  - Any ".." segments are resolved
 *  - All trailing slashes are removed
 * Allocates the result on the heap, free it when done.
 * @param path  to resolve
 * @return      resolved path or NULL if an error
 */
char* fs_resolve(const char* path);

/**
 * Resolve a path relative to a specific directory. This works the same
 * as the fs_resolve function, uses the specified cwd to resolve relative
 * paths instead of using the current working directory.
 * Allocates the result on the heap, free it when done.
 * @param cwd   to use when resolving a relative path
 * @param path  to resolve
 * @return      resolved path or NULL if an error
 */
char* fs_resolve_cwd(const char* cwd, const char* path);

/**
 * Joins two paths and does the following to normalize the result:
 *  - Multiple sequential slashes are replaced by a single slash
 *  - Any "." segments in the path are removed
 *  - Any ".." segments are resolved (if possible)
 *  - All trailing slashes are removed
 * Allocates the result on the heap, free it when done.
 * @param a  first path
 * @param b  second path
 * @return   joined result or NULL if an error
 */
char* fs_path_join(const char* a, const char* b);

/**
 * Determines the basename of the provided path. Does not mutate the original
 * path. Allocates the result on the heap, free it when done.
 * @param path  to find the basename of
 * @return      basename or NULL if an error
 */
char* fs_basename(char* path);

/**
 * Determines the parent directory of the provided path. Does not mutate the
 * original path. Allocates the result on the heap, free it when done.
 * @param path  to find the dirname of
 * @return      dirname or NULL if an error
 */
char* fs_dirname(char* path);

#endif
