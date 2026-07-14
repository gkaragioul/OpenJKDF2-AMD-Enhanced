#include "General/ConfigRecovery.h"

#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#endif

static bool config_recovery_replace(const char* temporary_path, const char* destination_path)
{
#if defined(_WIN32)
    return MoveFileExA(temporary_path, destination_path, MOVEFILE_REPLACE_EXISTING) != 0;
#else
    return rename(temporary_path, destination_path) == 0;
#endif
}

static bool config_recovery_copy_atomic(const char* source_path, const char* destination_path)
{
    char temporary_path[1024];
    unsigned char buffer[16384];
    FILE* source;
    FILE* destination;
    size_t count;
    int written;
    bool success = true;

    if (!source_path || !source_path[0] || !destination_path || !destination_path[0]) return false;
    written = snprintf(temporary_path, sizeof(temporary_path), "%s.tmp", destination_path);
    if (written < 0 || (size_t)written >= sizeof(temporary_path)) return false;

    source = fopen(source_path, "rb");
    if (!source) return false;
    destination = fopen(temporary_path, "wb");
    if (!destination) {
        fclose(source);
        return false;
    }

    while ((count = fread(buffer, 1, sizeof(buffer), source)) != 0) {
        if (fwrite(buffer, 1, count, destination) != count) {
            success = false;
            break;
        }
    }
    if (ferror(source)) success = false;
    if (fflush(destination) != 0) success = false;
    if (fclose(destination) != 0) success = false;
    if (fclose(source) != 0) success = false;

    if (success) success = config_recovery_replace(temporary_path, destination_path);
    if (!success) remove(temporary_path);
    return success;
}

bool config_recovery_has_snapshot(const char* snapshot_path)
{
    FILE* stream;
    if (!snapshot_path || !snapshot_path[0]) return false;
    stream = fopen(snapshot_path, "rb");
    if (!stream) return false;
    fclose(stream);
    return true;
}

bool config_recovery_should_offer(bool previous_run_unclean, bool snapshot_exists, bool safe_mode_requested)
{
    return previous_run_unclean && snapshot_exists && !safe_mode_requested;
}

bool config_recovery_snapshot(const char* active_path, const char* snapshot_path)
{
    return config_recovery_copy_atomic(active_path, snapshot_path);
}

bool config_recovery_restore(const char* snapshot_path, const char* active_path)
{
    return config_recovery_copy_atomic(snapshot_path, active_path);
}
