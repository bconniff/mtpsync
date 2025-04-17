#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>

#include "main/list.h"
#include "main/mtp.h"
#include "main/mtp_pull.h"
#include "main/mtp_push.h"
#include "main/mtp_ls.h"
#include "main/mtp_rm.h"
#include "main/mtp_devices.h"
#include "main/str.h"
#include "main/fs.h"
#include "main/io.h"
#include "main/args.h"

typedef struct {
    char* device_id;
    char* storage_id;
    int cleanup;
} MtpArgs;

typedef MtpStatusCode (*CommandFn)(int, char**, MtpArgs*);

typedef struct {
    char* cmd_name;
    CommandFn cmd_fn;
} Command;

static inline MtpDeviceParams as_params(MtpArgs* args) {
    MtpDeviceParams params = {
        .device_id = args->device_id,
        .storage_id = args->storage_id,
    };
    return params;
}

static void usage(char* name) {
    fprintf(stderr, "USAGE: %s <push|pull|rm> [options..] <path/to/file..>\n\n", name);
    fprintf(stderr, "    Sync files between filesystem and an MTP device\n\n");
    fprintf(stderr, "OPTIONS:\n\n");
    fprintf(stderr, "    -d [device_id]   Operate on a specific device ID\n");
    fprintf(stderr, "    -s [storage_id]  Operate on a specific storage volume\n");
    fprintf(stderr, "    -x               Remove stray files from target after push\n");
    fprintf(stderr, "COMMANDS:\n\n");
    fprintf(stderr, "    devices  Show available devices\n");
    fprintf(stderr, "    ls       List files and folders on the device\n");
    fprintf(stderr, "    push     Sends local files/folders to the device\n");
    fprintf(stderr, "    pull     Pulls files/folders from device\n");
    fprintf(stderr, "    rm       Deletes files or folders from the device\n\n");
}

static MtpStatusCode pull_impl(int argc, char **argv, MtpArgs* args) {
    if (argc < 3) {
        fprintf(stderr, "Specify a path to pull\n");
        return MTP_STATUS_ESYNTAX;
    }

    char* from_path = argv[2];
    char* to_path = argc < 4 ? NULL : argv[3];

    MtpDeviceParams params = as_params(args);
    return mtp_pull(&params, from_path, to_path);
}

static MtpStatusCode push_impl(int argc, char** argv, MtpArgs* args) {
    if (argc < 4) {
        fprintf(stderr, "Specify a source and target path\n");
        return MTP_STATUS_ESYNTAX;
    }

    char* from_path = argv[2];
    char* to_path = argv[3];

    MtpDeviceParams params = as_params(args);
    return mtp_push(&params, from_path, to_path, args->cleanup);
}

static MtpStatusCode rm_impl(int argc, char** argv, MtpArgs* args) {
    MtpStatusCode code = MTP_STATUS_ENOMEM;
    List* rm_paths = NULL;

    if (argc < 3) {
        fprintf(stderr, "Specify at least one path to delete\n");
        code = MTP_STATUS_ESYNTAX;
        goto done;
    }

    rm_paths = list_new(argc - 2);
    if (!rm_paths) goto done;

    for (int i = 2; i < argc; i++) {
        if (list_push(rm_paths, argv[i]) != LIST_STATUS_OK) goto done;
    }

    MtpDeviceParams params = as_params(args);
    code = mtp_rm(&params, rm_paths);

done:
    list_free(rm_paths);
    return code;
}

static MtpStatusCode ls_impl(int argc, char** argv, MtpArgs* args) {
    if (argc < 3) {
        fprintf(stderr, "Specify a path to list\n");
        return MTP_STATUS_ESYNTAX;
    }

    MtpDeviceParams params = as_params(args);
    return mtp_ls(&params, argv[2]);
}

static MtpStatusCode devices_impl(int argc, char** argv, MtpArgs* args) {
    MtpDeviceParams params = as_params(args);
    return mtp_devices(&params);
}

static ArgStatusCode device_arg(int argc, char** argv, int* i, void* data) {
    MtpArgs* args = data;

    if (++(*i) >= argc) {
        fprintf(stderr, "Please specify a device ID\n");
        return ARG_STATUS_ESYNTAX;
    }

    args->device_id = argv[*i];
    return ARG_STATUS_OK;
}

static ArgStatusCode storage_arg(int argc, char** argv, int* i, void* data) {
    MtpArgs* args = data;

    if (++(*i) >= argc) {
        fprintf(stderr, "Please specify a storage ID\n");
        return ARG_STATUS_ESYNTAX;
    }

    args->storage_id = argv[*i];
    return ARG_STATUS_OK;
}

static ArgStatusCode cleanup_arg(int argc, char** argv, int* i, void* data) {
    MtpArgs* args = data;
    args->cleanup = 1;
    return ARG_STATUS_OK;
}

static MtpStatusCode mtpsync(int argc, char** argv, MtpArgs* args) {
    MtpStatusCode code = MTP_STATUS_ENOCMD;

    Command cmds[] = {
        { .cmd_name = "devices", .cmd_fn = devices_impl },
        { .cmd_name = "ls", .cmd_fn = ls_impl },
        { .cmd_name = "push", .cmd_fn = push_impl },
        { .cmd_name = "pull", .cmd_fn = pull_impl },
        { .cmd_name = "rm", .cmd_fn = rm_impl },
    };

    if (argc < 2) {
        usage(argv[0]);
        return MTP_STATUS_OK;
    }

    char* cmd_name = argv[1];
    size_t cmds_size = sizeof(cmds)/sizeof(Command);
    for (size_t i = 0; i < cmds_size; i++) {
        Command c = cmds[i];
        if (strcmp(c.cmd_name, cmd_name) == 0) {
            code = c.cmd_fn(argc, argv, args);
            break;
        }
    }

    return code;
}

int main(int argc, char** argv) {
    MtpStatusCode code = MTP_STATUS_EFAIL;
    MtpArgs args = {0};

    ArgDefinition defs[] = {
        { .arg_long = "cleanup", .arg_short = 'x', .arg_fn = cleanup_arg },
        { .arg_long = "device", .arg_short = 'd', .arg_fn = device_arg },
        { .arg_long = "storage", .arg_short = 's', .arg_fn = storage_arg },
    };

    size_t def_count = sizeof(defs)/sizeof(ArgDefinition);
    ArgParseResult result = arg_parse(argc, argv, defs, def_count, &args);

    if (result.status == ARG_STATUS_OK) {
        code = mtpsync(result.argc, result.argv, &args);
        free(result.argv);
    }

    switch (code) {
        case MTP_STATUS_OK:
            return EXIT_SUCCESS;

        case MTP_STATUS_ENOCMD:
            fprintf(stderr, "Please specify a valid command.\n");
            break;

        case MTP_STATUS_ESYNTAX:
            fprintf(stderr, "Invalid syntax.\n");
            break;

        case MTP_STATUS_EREJECT:
            fprintf(stderr, "Action declined by user, exiting.\n");
            break;

        default:
            fprintf(stderr, "An unexpected error occurred (code %i).\n", code);
            break;
    }

    exit(code);
    return EXIT_FAILURE;
}
