/**
 * @file file.h
 * Simple wrapper object for dealing with files.
 */
#ifndef _FILE_H_
#define _FILE_H_

#include "list.h"
#include "hash.h"

/**
 * A file or folder. May be on the local filesystem or the remote device.
 */
typedef struct {
    char* path;    ///< Canonical path of the file
    int is_folder; ///< Truthy if this is a folder
    void* data;    ///< Additional data related to the file
} File;

/**
 * Free a File.
 * @param f  file to free
 */
void file_free(File* f);

/**
 * Create a new File. Returns NULL in case of failure. Free it when done.
 * @param path       canonical path of the file, will be copied
 * @param is_folder  truthy if this is a folder
 * @return           a new File, or NULL in case of an error
 */
File* file_new(const char* path, const int is_folder);

/**
 * Create a new File with attachd data. Returns NULL in case of failure.
 * Free it when done.
 * @param path       canonical path of the file, will be copied
 * @param is_folder  truthy if this is a folder
 * @param data       pointer to additional data to store with the file
 * @return           a new File, or NULL in case of an error
 */
File* file_new_data(const char* path, const int is_folder, void* data);

/**
 * Duplicates a File. The path will also be duplicated. Returns NULL in case of
 * failure. Free it when done.
 * @param f  to duplicate
 * @return   a copy of the file, or NULL in case of an error
 */
File* file_dup(File* f);

/**
 * Create a hashcode for a file based on path.
 * @param item  to create a hashcode for
 * @return      hashcode based on file path
 */
size_t file_hc(void* item);

/**
 * Compare the paths of two files
 * @param a  to compare
 * @param b  to compare
 * @return   zero if the paths are equal
 */
int file_cmp(void* a, void* b);

/**
 * Creates a filtered list of unique files based on path.
 * @param files  to filter
 * @return       new list containing the unique files
 */
List* file_unique(List* files);

/**
 * Frees a hash entry containing a File.
 * @param e  to free
 */
void file_hash_entry_free(HashEntry* e);

#endif
