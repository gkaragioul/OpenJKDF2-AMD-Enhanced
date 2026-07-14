#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "Platform/Win32/DisplayRestore.h"

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

static bool watchdog_write_proof(const wchar_t* path, const char* wait_result, bool restored, unsigned count)
{
    FILE* stream = NULL;
    if (!path || _wfopen_s(&stream, path, L"wb") != 0 || !stream) return false;
    fprintf(stream, "{\"schema\":1,\"wait\":\"%s\",\"restore\":\"%s\",\"display_count\":%u,\"topology_result\":%ld,\"display_result\":%ld}\n",
            wait_result, restored ? "success" : "failed", count,
            display_restore_topology_result(), display_restore_last_result());
    return fclose(stream) == 0;
}

int wmain(int argc, wchar_t** argv)
{
    if (argc == 3 && wcscmp(argv[1], L"--capture") == 0) {
        return display_restore_capture(argv[2]) ? 0 : 2;
    }
    if (argc == 3 && wcscmp(argv[1], L"--disarm") == 0) {
        return display_restore_disarm(argv[2]) ? 0 : 3;
    }
    if (argc == 3 && wcscmp(argv[1], L"--restore") == 0) {
        unsigned count = 0;
        return display_restore_apply_if_armed(argv[2], &count) ? 0 : 4;
    }
    if (argc == 5 && wcscmp(argv[1], L"--watch") == 0) {
        DWORD parent_id = wcstoul(argv[2], NULL, 10);
        HANDLE parent = OpenProcess(SYNCHRONIZE, FALSE, parent_id);
        const char* wait_result = "already_exited";
        unsigned count = 0;
        bool restored;
        if (parent) {
            DWORD wait = WaitForSingleObject(parent, INFINITE);
            wait_result = wait == WAIT_OBJECT_0 ? "parent_exit" : "wait_failed";
            CloseHandle(parent);
        }
        restored = display_restore_apply_if_armed(argv[3], &count);
        watchdog_write_proof(argv[4], wait_result, restored, count);
        return restored ? 0 : 5;
    }
    fwprintf(stderr, L"Usage: %ls --watch <pid> <state> <proof>\n", argv[0]);
    return 1;
}
