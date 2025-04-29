#define _XOPEN_SOURCE 500

#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <threads.h>
#include <unistd.h>
#include <ftw.h>

#include "fs.h"
#include "str.h"
#include "list.h"

#define FS_FILE_BUF_SIZE 128
#define FS_DIR_BUF_SIZE 32
#define FS_OPEN_FILES 16

enum FsPathState {
    FS_PATH_START,
    FS_PATH_NAME,
    FS_PATH_SEP,
    FS_PATH_DOT,
    FS_PATH_DOTDOT,
};

static thread_local List* files = NULL;

static inline int nftw_callback(const char* fpath, const struct stat* s, int tflag, struct FTW* ftwbuf) {
    int e = ENOMEM;
    char* fpath_r = NULL;

    if (!S_ISDIR(s->st_mode)) {
        fpath_r = fs_resolve(fpath);
        if (!fpath_r) goto done;

        if (list_push(files, fpath_r) != LIST_STATUS_OK) goto done;
        fpath_r = NULL;
    }

    e = 0;

done:
    free(fpath_r);
    return e;
}

static FsStatusCode handle_ancestor(List* parents, char* path) {
    FsStatusCode status = FS_STATUS_EFAIL;
    char* path_dup = NULL;

    struct stat s;
    int lstat_code = lstat(path, &s);

    if (lstat_code == 0) {
        path_dup = strdup(path);
        if (!path_dup) goto error;
        if (list_push(parents, path_dup) != LIST_STATUS_OK) goto error;
        path_dup = NULL;
        status = FS_STATUS_OK;
    } else if (errno == ENOENT) {
        status = FS_STATUS_ENOENT;
    }

error:
    free(path_dup);
    return status;
}

List* fs_collect_ancestors(char* path) {
    List* ancestors = NULL;
    char* root_dup = NULL;
    char* path_r = NULL;

    path_r = fs_resolve(path);
    if (!path_r) goto error;

    ancestors = list_new(FS_DIR_BUF_SIZE);
    if (!ancestors) goto error;

    root_dup = strdup("/");
    if (!root_dup) goto error;

    if (list_push(ancestors, root_dup) != LIST_STATUS_OK) goto error;
    root_dup = NULL;

    for (char* p = path_r+1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            FsStatusCode code = handle_ancestor(ancestors, path_r);
            *p = '/';

            if (code == FS_STATUS_ENOENT) goto done;
            if (code != FS_STATUS_OK) goto error;
        }
    }

    FsStatusCode code = handle_ancestor(ancestors, path_r);
    if (code == FS_STATUS_ENOENT || code == FS_STATUS_OK) goto done;

error:
    list_free_deep(ancestors, free);
    ancestors = NULL;

done:
    free(root_dup);
    free(path_r);
    return ancestors;

}

List* fs_collect_files(char *path) {
    int e = ENOMEM;
    char* path_r = NULL;
    files = NULL;

    struct stat s;
    int lstat_code = lstat(path, &s);
    if (lstat_code != 0) {
        e = errno; // save the errno from lstat
        goto error;
    }

    files = list_new(FS_FILE_BUF_SIZE);
    if (!files) goto error;

    if (S_ISDIR(s.st_mode)) {
        int nftw_code = nftw(path, nftw_callback, FS_OPEN_FILES, 0);
        if (nftw_code != 0) {
            e = nftw_code;
            goto error;
        }
    } else {
        char* path_r = fs_resolve(path);
        if (!path_r) goto error;
        if (list_push(files, path_r) != LIST_STATUS_OK) goto error;
        path_r = NULL;
    }

    return files;

error:
    free(path_r);
    list_free_deep(files, free);
    errno = e;
    return NULL;
}

FsStatusCode fs_rm(char* path) {
    struct stat s;
    int lstat_code = lstat(path, &s);

    if (lstat_code != 0) return FS_STATUS_EFAIL;

    if (S_ISDIR(s.st_mode)) {
        if (rmdir(path) != 0) return FS_STATUS_EFAIL;
    } else {
        if (unlink(path) != 0) return FS_STATUS_EFAIL;
    }

    return FS_STATUS_OK;
}

FsStatusCode fs_mkdir(char* path) {
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
            if (fs_mkdir(tmp) != FS_STATUS_OK) goto done;
            *p = '/';
        }
    }

    if (fs_mkdir(tmp) != FS_STATUS_OK) goto done;

    result = FS_STATUS_OK;

done:
    free(tmp);
    return result;
}

size_t fs_path_append(char* result, const size_t len, const char* path) {
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

char* fs_resolve_cwd(const char* cwd, const char* path) {
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

char* fs_resolve(const char* path) {
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

char* fs_path_join(const char* head, const char* tail) {
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
