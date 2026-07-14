#define WIN32_LEAN_AND_MEAN
#undef WINVER
#undef _WIN32_WINNT
#define WINVER 0x0601
#define _WIN32_WINNT 0x0601
#include <windows.h>

#include "Platform/Win32/DisplayRestore.h"

#include <stdio.h>
#include <string.h>

#define DISPLAY_RESTORE_MAGIC 0x52444A4Fu
#define DISPLAY_RESTORE_VERSION 2u
#define DISPLAY_RESTORE_MAX_DISPLAYS 16u
#define DISPLAY_RESTORE_MAX_PATHS 32u
#define DISPLAY_RESTORE_MAX_MODES 64u

typedef struct DisplayRestoreEntry
{
    wchar_t device_name[CCHDEVICENAME];
    DEVMODEW mode;
} DisplayRestoreEntry;

typedef struct DisplayRestoreState
{
    unsigned magic;
    unsigned version;
    unsigned armed;
    unsigned count;
    unsigned path_count;
    unsigned mode_count;
    DisplayRestoreEntry entries[DISPLAY_RESTORE_MAX_DISPLAYS];
    DISPLAYCONFIG_PATH_INFO paths[DISPLAY_RESTORE_MAX_PATHS];
    DISPLAYCONFIG_MODE_INFO modes[DISPLAY_RESTORE_MAX_MODES];
} DisplayRestoreState;

static LONG display_restore_result = DISP_CHANGE_SUCCESSFUL;
static LONG display_restore_topology = ERROR_SUCCESS;

long display_restore_last_result(void)
{
    return display_restore_result;
}

long display_restore_topology_result(void)
{
    return display_restore_topology;
}

static bool display_restore_read(const wchar_t* path, DisplayRestoreState* state)
{
    FILE* stream = NULL;
    size_t read_count;
    if (!path || !state || _wfopen_s(&stream, path, L"rb") != 0 || !stream) return false;
    read_count = fread(state, sizeof(*state), 1, stream);
    fclose(stream);
    return read_count == 1 && state->magic == DISPLAY_RESTORE_MAGIC &&
           state->version == DISPLAY_RESTORE_VERSION && state->count <= DISPLAY_RESTORE_MAX_DISPLAYS &&
           state->path_count <= DISPLAY_RESTORE_MAX_PATHS && state->mode_count <= DISPLAY_RESTORE_MAX_MODES;
}

static bool display_restore_write(const wchar_t* path, const DisplayRestoreState* state)
{
    wchar_t temporary[MAX_PATH * 2];
    FILE* stream = NULL;
    int length;
    if (!path || !state) return false;
    length = _snwprintf_s(temporary, sizeof(temporary) / sizeof(temporary[0]), _TRUNCATE, L"%ls.tmp", path);
    if (length < 0 || _wfopen_s(&stream, temporary, L"wb") != 0 || !stream) return false;
    if (fwrite(state, sizeof(*state), 1, stream) != 1 || fflush(stream) != 0) {
        fclose(stream);
        DeleteFileW(temporary);
        return false;
    }
    if (fclose(stream) != 0) {
        DeleteFileW(temporary);
        return false;
    }
    return MoveFileExW(temporary, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
}

bool display_restore_capture(const wchar_t* state_path)
{
    DisplayRestoreState state;
    DWORD device_index;
    UINT32 path_count = 0;
    UINT32 mode_count = 0;
    memset(&state, 0, sizeof(state));
    state.magic = DISPLAY_RESTORE_MAGIC;
    state.version = DISPLAY_RESTORE_VERSION;
    state.armed = 1;
    if (GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &path_count, &mode_count) == ERROR_SUCCESS &&
        path_count <= DISPLAY_RESTORE_MAX_PATHS && mode_count <= DISPLAY_RESTORE_MAX_MODES) {
        state.path_count = path_count;
        state.mode_count = mode_count;
        if (QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &path_count, state.paths, &mode_count, state.modes, NULL) != ERROR_SUCCESS) {
            state.path_count = 0;
            state.mode_count = 0;
        } else {
            state.path_count = path_count;
            state.mode_count = mode_count;
        }
    }
    for (device_index = 0; device_index < DISPLAY_RESTORE_MAX_DISPLAYS; ++device_index) {
        DISPLAY_DEVICEW device;
        DisplayRestoreEntry* entry;
        memset(&device, 0, sizeof(device));
        device.cb = sizeof(device);
        if (!EnumDisplayDevicesW(NULL, device_index, &device, 0)) break;
        if (!(device.StateFlags & DISPLAY_DEVICE_ACTIVE) || !(device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)) continue;
        entry = &state.entries[state.count];
        wcsncpy_s(entry->device_name, CCHDEVICENAME, device.DeviceName, _TRUNCATE);
        memset(&entry->mode, 0, sizeof(entry->mode));
        entry->mode.dmSize = sizeof(entry->mode);
        if (!EnumDisplaySettingsExW(entry->device_name, ENUM_CURRENT_SETTINGS, &entry->mode, 0)) continue;
        ++state.count;
    }
    return state.count > 0 && state.path_count > 0 && display_restore_write(state_path, &state);
}

bool display_restore_disarm(const wchar_t* state_path)
{
    DisplayRestoreState state;
    if (!display_restore_read(state_path, &state)) return false;
    state.armed = 0;
    return display_restore_write(state_path, &state);
}

bool display_restore_apply_if_armed(const wchar_t* state_path, unsigned* restored_displays)
{
    DisplayRestoreState state;
    unsigned index;
    bool success = true;
    if (restored_displays) *restored_displays = 0;
    display_restore_result = DISP_CHANGE_SUCCESSFUL;
    display_restore_topology = ERROR_SUCCESS;
    if (!display_restore_read(state_path, &state)) return false;
    if (!state.armed) return true;
    {
        LONG topology_result = SetDisplayConfig(state.path_count, state.paths, state.mode_count, state.modes,
            SDC_APPLY | SDC_USE_SUPPLIED_DISPLAY_CONFIG);
        display_restore_result = topology_result;
        display_restore_topology = topology_result;
        if (topology_result == ERROR_SUCCESS) {
            if (restored_displays) *restored_displays = state.count;
            state.armed = 0;
            return display_restore_write(state_path, &state);
        }
    }
    for (index = 0; index < state.count; ++index) {
        LONG result = ChangeDisplaySettingsExW(state.entries[index].device_name, &state.entries[index].mode,
                                               NULL, 0, NULL);
        if (result != DISP_CHANGE_SUCCESSFUL) {
            result = ChangeDisplaySettingsExW(state.entries[index].device_name, NULL, NULL, 0, NULL);
        }
        if (result == DISP_CHANGE_SUCCESSFUL) {
            if (restored_displays) ++*restored_displays;
        } else {
            display_restore_result = result;
            success = false;
        }
    }
    if (!success) {
        LONG global_result = ChangeDisplaySettingsExW(NULL, &state.entries[0].mode, NULL, 0, NULL);
        if (global_result != DISP_CHANGE_SUCCESSFUL) {
            global_result = ChangeDisplaySettingsExW(NULL, NULL, NULL, 0, NULL);
        }
        display_restore_result = global_result;
        if (global_result == DISP_CHANGE_SUCCESSFUL) {
            success = true;
            if (restored_displays) *restored_displays = state.count;
        }
    }
    if (success) {
        state.armed = 0;
        success = display_restore_write(state_path, &state);
    }
    return success;
}
