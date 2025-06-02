/**
 * @file mtp_pull.h
 * Implements the "pull" sub-command.
 */

#ifndef _MTP_PULL_H_
#define _MTP_PULL_H_

/**
 * Implements the "pull" sub-command.
 * @param args       command-line arguments
 * @param from_path  path on the device to retrieve files from
 * @param to_path    local path to retrieve files into
 * @return           status code of the operation
 */
MtpStatusCode mtp_pull(MtpArgs* args, char* from_path, char* to_path);

#endif
