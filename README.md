# mtpsync

Command-line tool to synchronize files with MTP devices, based on libmtp.
Designed to work with Garmin watches, not tested with other devices.

## Compiling

First, make sure you have `libmtp` installed.

Build `mtpsync` with make:

```sh
make
```

This will produce the `bin/mtpsync` executable. Copy it wherever you'd like.

## Examples

There are a variety of sub-commands available.

First, use `mtpsync devices` to see your available devices.

```sh
mtpsync devices
```

This will show your attached devices, as such:

```
Device: Venu 2S
 * Number: 0
 * Serial: SN:0000ca691cde
 * Storage: Internal Storage
   - ID: 00020001
   - Free Space: 79% (6076923904 bytes)
```

Now, use the `ls`, `pull`, `push` and `rm` commands to interact with your devices:

```
# list files on all attached devices, by default
mtpsync ls /

# or, specify a device (by serial number or index) and storage ID
# all sub-commands accept the -d and -s options
mtpsync ls / -d SN:000ca691cde -s 00020001
mtpsync ls / -d 0 -s 00020001

# push a local file or directory to an attached MTP device
mtpsync push local/path /remote/path

# add the -x flag if you'd like to delete any stray files from the target folder
mtpsync push local/path /remote/path -x

# retreive a file or directory from the MTP device to a local folder
mtpsync pull /remote/path local/path

# remove a file or recursively delete a folder on the device
mtpsync rm /remote/path
mtpsync rm /remote/path/a /remote/path/b /remote/path/c
```
