/**
 * @file mtp_push.h
 * Implements the "push" sub-command.
 */

#ifndef _MTP_PUSH_H_
#define _MTP_PUSH_H_

/**
 * Implements the "push" sub-command.
 * @param params     command-line parameters
 * @param from_path  local path to push files from
 * @param to_path    path on the device to send files to
 * @param cleanup    if truthy, delete stray files from the to_path
 * @return           status code of the operation
 */
MtpStatusCode mtp_push(MtpDeviceParams* params, char* from_path, char* to_path, int cleanup);

#endif
