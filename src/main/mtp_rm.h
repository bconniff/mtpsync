/**
 * @file mtp_rm.h
 * Implements the "rm" sub-command.
 */

#ifndef _MTP_RM_H_
#define _MTP_RM_H_

/**
 * Implements the "rm" sub-command.
 * @param args   command-line arguments
 * @param paths  list of paths to delete from the device
 * @return       status code of the operation
 */
MtpStatusCode mtp_rm(MtpArgs* args, List* paths);

#endif
