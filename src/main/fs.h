#ifndef _FS_H_
#define _FS_H_

#include "list.h"

#define FS_FILE_BUF_SIZE 128
#define FS_DIR_BUF_SIZE 32

typedef enum {
    FS_STATUS_OK,
    FS_STATUS_ENOMEM,
    FS_STATUS_EFAIL,
} FsStatusCode;

List* fs_collect_files(char* path);
FsStatusCode fs_mkdirp(char* path);
char* fs_resolve(char* path);
char* fs_resolve_cwd(char* cwd, char* path);
char* fs_path_join(char* a, char* b);
size_t fs_path_append(char* result, size_t len, char* path);
char* fs_basename(char* path);
char* fs_dirname(char* path);

#endif
