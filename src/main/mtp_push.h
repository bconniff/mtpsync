/**
 * @file mtp_push.h
 * Implements the "push" sub-command.
 */

#ifndef _MTP_PUSH_H_
#define _MTP_PUSH_H_

/**
 * Implements the "push" sub-command.
 * @param args       command-line arguments
 * @param from_path  local path to push files from
 * @param to_path    path on the device to send files to
 * @return           status code of the operation
 */
MtpStatusCode mtp_push(MtpArgs* args, char* from_path, char* to_path);

#endif
