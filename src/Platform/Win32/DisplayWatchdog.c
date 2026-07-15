#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "Platform/Win32/DisplayWatchdog.h"
#include "Platform/Win32/DisplayRestore.h"

#include <stdio.h>
#include <string.h>
#include <wchar.h>

#define DISPLAY_WATCHDOG_PATH_CAPACITY 4096
#define DISPLAY_WATCHDOG_COMMAND_CAPACITY 16384
#define DISPLAY_WATCHDOG_READY_TIMEOUT_MS 5000u

static wchar_t watchdog_state_path[DISPLAY_WATCHDOG_PATH_CAPACITY];
static wchar_t watchdog_proof_path[DISPLAY_WATCHDOG_PATH_CAPACITY];
static wchar_t watchdog_ready_path[DISPLAY_WATCHDOG_PATH_CAPACITY];
static const char* watchdog_status = "not_started";
static long watchdog_topology_result = ERROR_SUCCESS;
static long watchdog_display_result = DISP_CHANGE_SUCCESSFUL;
static bool watchdog_active = false;

static bool watchdog_join_path(wchar_t* output, size_t output_count,
                               const wchar_t* directory, const wchar_t* leaf)
{
    size_t length;
    if (!output || !output_count || !directory || !leaf) return false;
    length = wcslen(directory);
    if (!length || length + wcslen(leaf) + 2 > output_count) return false;
    if (wcscpy_s(output, output_count, directory) != 0) return false;
    if (output[length - 1] != L'\\' && output[length - 1] != L'/') {
        if (wcscat_s(output, output_count, L"\\") != 0) return false;
    }
    return wcscat_s(output, output_count, leaf) == 0;
}

static bool watchdog_absolute_directory(const char* input, wchar_t* output, size_t output_count)
{
    wchar_t converted[DISPLAY_WATCHDOG_PATH_CAPACITY];
    int converted_length;
    DWORD full_length;
    if (!input || !input[0] || !output || !output_count) return false;
    converted_length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input, -1,
                                            converted, DISPLAY_WATCHDOG_PATH_CAPACITY);
    if (!converted_length) {
        converted_length = MultiByteToWideChar(CP_ACP, 0, input, -1,
                                                converted, DISPLAY_WATCHDOG_PATH_CAPACITY);
    }
    if (!converted_length) return false;
    full_length = GetFullPathNameW(converted, (DWORD)output_count, output, NULL);
    return full_length > 0 && full_length < output_count;
}

static bool watchdog_build_paths(const char* diagnostics_dir)
{
    wchar_t directory[DISPLAY_WATCHDOG_PATH_CAPACITY];
    if (!watchdog_absolute_directory(diagnostics_dir, directory,
                                     sizeof(directory) / sizeof(directory[0]))) return false;
    return watchdog_join_path(watchdog_state_path,
                              sizeof(watchdog_state_path) / sizeof(watchdog_state_path[0]),
                              directory, L"display-watchdog-state.bin") &&
           watchdog_join_path(watchdog_proof_path,
                              sizeof(watchdog_proof_path) / sizeof(watchdog_proof_path[0]),
                              directory, L"display-watchdog-proof.json") &&
           watchdog_join_path(watchdog_ready_path,
                              sizeof(watchdog_ready_path) / sizeof(watchdog_ready_path[0]),
                              directory, L"display-watchdog-ready.json");
}

static bool watchdog_locate_helper(wchar_t* output, size_t output_count)
{
    static const wchar_t* candidates[] = {
        L"OpenJKDF2-Display-Watchdog.exe",
        L"openjkdf2-display-watchdog.exe"
    };
    wchar_t module_path[DISPLAY_WATCHDOG_PATH_CAPACITY];
    wchar_t* separator;
    DWORD length = GetModuleFileNameW(NULL, module_path,
                                      (DWORD)(sizeof(module_path) / sizeof(module_path[0])));
    size_t index;
    if (!length || length >= sizeof(module_path) / sizeof(module_path[0])) return false;
    separator = wcsrchr(module_path, L'\\');
    if (!separator) separator = wcsrchr(module_path, L'/');
    if (!separator) return false;
    *separator = L'\0';
    for (index = 0; index < sizeof(candidates) / sizeof(candidates[0]); ++index) {
        if (watchdog_join_path(output, output_count, module_path, candidates[index]) &&
            GetFileAttributesW(output) != INVALID_FILE_ATTRIBUTES) return true;
    }
    return false;
}

static bool watchdog_build_command(wchar_t* output, size_t output_count,
                                   const wchar_t* helper_path)
{
    if (!output || !output_count || !helper_path ||
        wcschr(helper_path, L'"') || wcschr(watchdog_state_path, L'"') ||
        wcschr(watchdog_proof_path, L'"') || wcschr(watchdog_ready_path, L'"')) return false;
    return _snwprintf_s(output, output_count, _TRUNCATE,
                        L"\"%ls\" --watch %lu \"%ls\" \"%ls\" \"%ls\"",
                        helper_path, (unsigned long)GetCurrentProcessId(), watchdog_state_path,
                        watchdog_proof_path, watchdog_ready_path) > 0;
}

static bool watchdog_fail(const char* status, HANDLE process)
{
    DWORD exit_code = 0;
    watchdog_status = status;
    watchdog_active = false;
    if (process && GetExitCodeProcess(process, &exit_code) && exit_code == STILL_ACTIVE) {
        TerminateProcess(process, 8);
        WaitForSingleObject(process, 1000);
    }
    if (process) CloseHandle(process);
    if (watchdog_state_path[0]) display_restore_disarm(watchdog_state_path);
    if (watchdog_ready_path[0]) DeleteFileW(watchdog_ready_path);
    return false;
}

bool display_watchdog_start(const char* diagnostics_dir)
{
    wchar_t helper_path[DISPLAY_WATCHDOG_PATH_CAPACITY];
    wchar_t command[DISPLAY_WATCHDOG_COMMAND_CAPACITY];
    STARTUPINFOW startup;
    PROCESS_INFORMATION process;
    DWORD elapsed = 0;
    unsigned restored_displays = 0;
    bool armed = false;

    if (watchdog_active) return true;
    watchdog_state_path[0] = L'\0';
    watchdog_proof_path[0] = L'\0';
    watchdog_ready_path[0] = L'\0';
    watchdog_status = "path_failed";
    watchdog_topology_result = ERROR_SUCCESS;
    watchdog_display_result = DISP_CHANGE_SUCCESSFUL;
    if (!watchdog_build_paths(diagnostics_dir)) return false;

    DeleteFileW(watchdog_state_path);
    DeleteFileW(watchdog_proof_path);
    DeleteFileW(watchdog_ready_path);
    if (GetFileAttributesW(watchdog_ready_path) != INVALID_FILE_ATTRIBUTES)
        return watchdog_fail("stale_ready_failed", NULL);

    if (!display_restore_capture(watchdog_state_path))
        return watchdog_fail("capture_failed", NULL);
    if (!display_restore_apply_if_armed(watchdog_state_path, &restored_displays)) {
        watchdog_topology_result = display_restore_topology_result();
        watchdog_display_result = display_restore_last_result();
        return watchdog_fail("preflight_failed", NULL);
    }
    watchdog_topology_result = display_restore_topology_result();
    watchdog_display_result = display_restore_last_result();
    if (!restored_displays) return watchdog_fail("preflight_empty", NULL);

    if (!display_restore_capture(watchdog_state_path))
        return watchdog_fail("recapture_failed", NULL);
    if (!watchdog_locate_helper(helper_path, sizeof(helper_path) / sizeof(helper_path[0])))
        return watchdog_fail("helper_missing", NULL);
    if (!watchdog_build_command(command, sizeof(command) / sizeof(command[0]), helper_path))
        return watchdog_fail("command_failed", NULL);

    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    memset(&process, 0, sizeof(process));
    if (!CreateProcessW(NULL, command, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL,
                        &startup, &process))
        return watchdog_fail("launch_failed", NULL);
    CloseHandle(process.hThread);

    while (elapsed < DISPLAY_WATCHDOG_READY_TIMEOUT_MS) {
        DWORD wait_result = WaitForSingleObject(process.hProcess, 25);
        if (wait_result == WAIT_OBJECT_0) return watchdog_fail("helper_exited", process.hProcess);
        if (wait_result == WAIT_FAILED) return watchdog_fail("helper_wait_failed", process.hProcess);
        elapsed += 25;
        if (GetFileAttributesW(watchdog_ready_path) != INVALID_FILE_ATTRIBUTES) {
            if (!display_restore_is_armed(watchdog_state_path, &armed) || !armed)
                return watchdog_fail("ready_state_invalid", process.hProcess);
            CloseHandle(process.hProcess);
            watchdog_active = true;
            watchdog_status = "ready";
            return true;
        }
    }
    return watchdog_fail("ready_timeout", process.hProcess);
}

bool display_watchdog_disarm(void)
{
    bool success = true;
    if (watchdog_state_path[0]) success = display_restore_disarm(watchdog_state_path);
    watchdog_active = false;
    if (watchdog_ready_path[0]) DeleteFileW(watchdog_ready_path);
    watchdog_status = success ? "disarmed" : "disarm_failed";
    return success;
}

const char* display_watchdog_status(void)
{
    return watchdog_status;
}

long display_watchdog_topology_result(void)
{
    return watchdog_topology_result;
}

long display_watchdog_display_result(void)
{
    return watchdog_display_result;
}
