/**
 * @file mtp_rm.h
 * Implements the "rm" sub-command.
 */

#ifndef _MTP_RM_H_
#define _MTP_RM_H_

/**
 * Remove a list of files. The user will be prompted interactively to confirm
 * the operation. This will delete files in the following order:
 *  - Regular files are deleted first, in alphabetic order
 *  - Directories are deleted last, in order of descending depth
 * @param dev    device to operate on
 * @param files  list of paths to delete from the device
 * @return       status code of the operation
 */
MtpStatusCode mtp_rm_files(Device* dev, List* files);

/**
 * Remove all files within the selected paths. The user will be prompted
 * interactively to confirm the operation. This will use the same deletion
 * order as mtp_rm_files.
 * @param dev    device to operate on
 * @param paths  of paths to delete from the device
 * @return       code of the operation
 */
MtpStatusCode mtp_rm_paths(Device* dev, List* paths);

/**
 * Implements the "rm" sub-command.
 * @param params  command-line parameters
 * @param paths   list of paths to delete from the device
 * @return        status code of the operation
 */
MtpStatusCode mtp_rm(MtpDeviceParams* params, List* paths);

#endif
