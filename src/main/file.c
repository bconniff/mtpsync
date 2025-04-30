#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>

#include "fs.h"
#include "file.h"
#include "hash.h"

inline size_t file_hc(void* item) {
    File* sf = item;
    return hash_code_str(sf->path);
}

inline int file_cmp(void* a, void* b) {
    File* aa = a;
    File* bb = b;
    return hash_cmp_str(aa->path, bb->path);
}

inline void file_hash_entry_free(HashEntry* e) {
    file_free(hash_entry_value(e));
    hash_entry_free(e);
}

void file_free(File* f) {
    if (f) {
        free(f->path);
        free(f);
    }
}

File* file_new_data(const char* path, const int is_folder, void* data) {
    char* path_dup = NULL;
    File* file = NULL;

    path_dup = fs_resolve(path);
    if (!path_dup) goto error;

    file = malloc(sizeof(File));
    if (!file) goto error;
    file->path = path_dup;
    file->is_folder = is_folder;
    file->data = data;
    return file;

error:
    free(path_dup);
    free(file);
    return NULL;
}

File* file_new(const char* path, const int is_folder) {
    return file_new_data(path, is_folder, NULL);
}

File* file_dup(File* f) {
    return f ? file_new_data(f->path, f->is_folder, f->data) : NULL;
}

List* file_unique(List* files) {
    return hash_unique(files, file_hc, file_cmp);
}
