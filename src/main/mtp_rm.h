#ifndef _MTP_RM_H_
#define _MTP_RM_H_

MtpStatusCode mtp_rm_paths(Device* dev, List* paths);
MtpStatusCode mtp_rm_files(Device* dev, List* files);
MtpStatusCode mtp_rm(MtpDeviceParams* mtp_params, List* path);

#endif
