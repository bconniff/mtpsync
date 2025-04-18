/**
 * @file mtp_pull.h
 * Implements the "pull" sub-command.
 */

#ifndef _MTP_PULL_H_
#define _MTP_PULL_H_

/**
 * Implements the "pull" sub-command.
 * @param params     command-line parameters
 * @param from_path  path on the device to retrieve files from
 * @param to_path    local path to retrieve files into
 * @return           status code of the operation
 */
MtpStatusCode mtp_pull(MtpDeviceParams* params, char* from_path, char* to_path);

#endif
