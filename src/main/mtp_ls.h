#ifndef _MTP_LS_H_
#define _MTP_LS_H_

/**
 * Implements the "ls" sub-command.
 * @param params   command-line parameters
 * @param ls_path  path to list contents of
 * @return         status code of the operation
 */
MtpStatusCode mtp_ls(MtpDeviceParams* params, char* ls_path);

#endif
