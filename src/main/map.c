#include "device.h"
#include "list.h"
#include "sync.h"
#include "map.h"

inline SyncFile* map_device_file_to_sync_file(DeviceFile* df, MapStatusCode* code) {
    SyncFile* sf = sync_file_new(df->path, df->is_folder);
    if (!sf) *code = MAP_STATUS_EFAIL;
    return sf;
}

inline SyncFile* map_sync_spec_to_sync_file(SyncSpec* spec, MapStatusCode* code) {
    SyncFile* sf = sync_file_new(spec->source, 0);
    if (!sf) *code = MAP_STATUS_EFAIL;
    return sf;
}
