/**
 * @file mtp_ls.h
 * Implements the "ls" sub-command.
 */
#ifndef _MTP_LS_H_
#define _MTP_LS_H_

/**
 * Implements the "ls" sub-command.
 * @param args     command-line parameters
 * @param ls_path  path to list contents of
 * @return         status code of the operation
 */
MtpStatusCode mtp_ls(MtpArgs* args, char* ls_path);

#endif
