#include <stdlib.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include "fs.h"
#include "str.h"
#include "list.h"

#define FS_FILE_BUF_SIZE 128
#define FS_DIR_BUF_SIZE 32

enum FsPathState {
    FS_PATH_START,
    FS_PATH_NAME,
    FS_PATH_SEP,
    FS_PATH_DOT,
    FS_PATH_DOTDOT,
};

static FsStatusCode fs_collect_files_recursive(List* files, char *path);

static FsStatusCode fs_collect_files_recursive(List* files, char *path) {
    FsStatusCode code = FS_STATUS_EFAIL;
    List* children = NULL;
    char* path_copy = NULL;
    char* child_path = NULL;
    DIR* dir = NULL;

    struct stat path_s;
    if (lstat(path, &path_s) != 0) goto done;

    if (!S_ISDIR(path_s.st_mode)) {
        path_copy = strdup(path);
        if (!path_copy) goto done;

        if (list_push(files, path_copy) != LIST_STATUS_OK) goto done;
        path_copy = NULL;
        code = FS_STATUS_OK;
        goto done;
    }

    children = list_new(FS_DIR_BUF_SIZE);
    if (!children) goto done;

    dir = opendir(path);
    if (!dir) goto done;

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        char* name = entry->d_name;
        char* child_path = NULL;

        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue;
        }

        child_path = fs_path_join(path, name);
        if (!child_path) goto done;

        if (list_push(children, child_path) != LIST_STATUS_OK) goto done;

        child_path = NULL;
    }

    closedir(dir);
    dir = NULL;

    for (size_t i = 0; i < list_size(children); i++) {
        char* child_path = list_get(children, i);
        fs_collect_files_recursive(files, child_path);
    }

    code = FS_STATUS_OK;

done:
    if (dir) closedir(dir);
    free(path_copy);
    free(child_path);
    list_free_deep(children, free);
    return code;
}

List* fs_collect_files(char *path) {
    List* files = NULL;

    files = list_new(FS_FILE_BUF_SIZE);
    if (!files) goto error;

    if (fs_collect_files_recursive(files, path) != FS_STATUS_OK) goto error;

    return files;

error:
    list_free_deep(files, free);
    return NULL;
}

static FsStatusCode fs_mkdir_try(char* path) {
    struct stat s;
    int lstat_code = lstat(path, &s);

    if (lstat_code == 0) {
        if (!S_ISDIR(s.st_mode)) {
            errno = ENOTDIR;
            return FS_STATUS_EFAIL;
        }
        return FS_STATUS_OK;
    }

    if (errno != ENOENT) {
        return FS_STATUS_EFAIL;
    }

    if (mkdir(path, S_IRWXU) != 0) {
        return FS_STATUS_EFAIL;
    }

    return FS_STATUS_OK;
}

FsStatusCode fs_mkdirp(char* path) {
    int result = FS_STATUS_EFAIL;
    char* tmp = fs_resolve(path);

    if (!tmp) {
        result = FS_STATUS_ENOMEM;
        goto done;
    }

    for (char* p = tmp+1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            if (fs_mkdir_try(tmp) != FS_STATUS_OK) goto done;
            *p = '/';
        }
    }

    if (fs_mkdir_try(tmp) != FS_STATUS_OK) goto done;

    result = FS_STATUS_OK;

done:
    free(tmp);
    return result;
}

size_t fs_path_append(char* result, size_t len, char* path) {
    size_t i = 0;
    size_t j = len;
    enum FsPathState state = FS_PATH_START;

    while (path && path[i]) {
        switch (state) {
            case FS_PATH_START:
                if (path[i] == '/') {
                    state = FS_PATH_SEP;
                } else if (strcmp(path+i, "..") == 0 || strncmp(path+i, "../", 3) == 0) {
                    state = FS_PATH_DOTDOT;
                } else if (strcmp(path+i, ".") == 0 || strncmp(path+i, "./", 2) == 0) {
                    state = FS_PATH_DOT;
                } else {
                    state = FS_PATH_NAME;
                }
                break;

            case FS_PATH_NAME:
                if (j == 1 && result[0] == '.') j--;
                if (j && result[j-1] != '/') result[j++] = '/';
                while (path[i] && path[i] != '/') result[j++] = path[i++];
                state = FS_PATH_START;
                break;

            case FS_PATH_SEP:
                while (path[i++] == '/');
                i--;
                if (!j) result[j++] = '/';
                state = FS_PATH_START;
                break;

            case FS_PATH_DOT:
                i++;
                if (!j) result[j++] = '.';
                result[j] = 0;
                state = FS_PATH_START;
                break;

            case FS_PATH_DOTDOT:
                i+=2;

                if (!j) result[j++] = '.';

                if (j == 1 && result[0] == '.') {
                    // if path is "." change it to ".."
                    result[j++] = '.';
                } else if (
                    (j == 2 && strncmp(result, "..", 2) == 0) ||
                    (j > 2 && strncmp(result+j-3, "/..", 3) == 0)
                ) {
                    // if path is ".." or ends in "/..", append "/.."
                    strcpy(result+j, "/..");
                    j += 3;
                } else {
                    // otherwise, reverse until we find a '/'
                    while (j && (result[--j] != '/'));
                    if (result[j] != '/') {
                        result[j] = '.';
                        j++;
                    }
                }
                state = FS_PATH_START;
                break;
        }
    }

    result[j] = 0;

    return j;
}

char* fs_resolve_cwd(char* cwd, char* path) {
    char* result = NULL;
    size_t path_len = path ? strlen(path) : 1;
    size_t j = 0;

    int is_relative = !path || path[0] != '/';

    // prepend cwd if not an absolute path
    if (cwd && is_relative) {
        path_len += 1 + strlen(cwd);
    }

    result = malloc( (1+path_len) * sizeof(char) );
    if (!result) goto error;

    if (cwd && is_relative) {
        j = fs_path_append(result, j, cwd);
    }
    j = fs_path_append(result, j, path);

    if (!j) strcpy(result, ".");

    return result;

error:
    free(result);
    return NULL;
}

char* fs_resolve(char* path) {
    char* result = NULL;
    char* cwd = NULL;

    if (!path || path[0] != '/') {
        cwd = getcwd(NULL, 0);
        if (!cwd) goto error;
    }

    result = fs_resolve_cwd(cwd, path);
    if (!result) goto error;

    free(cwd);
    return result;

error:
    free(cwd);
    free(result);
    return NULL;
}

char* fs_path_join(char* head, char* tail) {
    int j = 0;
    size_t head_len = head ? strlen(head) : 0;
    size_t tail_len = tail ? strlen(tail) : 0;
    char* result = malloc((head_len+tail_len+2) * sizeof(char));
    if (!result) return NULL;

    j = fs_path_append(result, j, head);
    j = fs_path_append(result, j, tail);

    if (!j) strcpy(result, ".");

    return result;
}

char* fs_basename(char* path) {
    if (!path) return NULL;
    char* path_dup = strdup(path);
    if (!path_dup) return NULL;
    char* bname = basename(path_dup);
    char* bname_dup = strdup(bname);
    free(path_dup);
    return bname_dup;
}

char* fs_dirname(char* path) {
    if (!path) return NULL;
    char* path_dup = strdup(path);
    if (!path_dup) return NULL;
    char* dname = dirname(path_dup);
    char* dname_dup = strdup(dname);
    free(path_dup);
    return dname_dup;
}
