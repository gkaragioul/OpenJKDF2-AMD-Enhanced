#include "General/StoragePaths.h"

#include <stdio.h>
#include <string.h>

static bool storage_copy(const char* value, char* output, size_t output_size)
{
    size_t length = value ? strlen(value) : 0;
    if (!length || !output || length >= output_size) return false;
    memcpy(output, value, length + 1);
    return true;
}

static bool storage_join(const char* root, size_t root_length, const char* leaf,
                         char* output, size_t output_size)
{
    int written;
    while (root_length && (root[root_length - 1] == '/' || root[root_length - 1] == '\\')) --root_length;
    written = snprintf(output, output_size, "%.*s%c%s", (int)root_length, root,
#ifdef _WIN32
                       '\\',
#else
                       '/',
#endif
                       leaf);
    return written >= 0 && (size_t)written < output_size;
}

bool storage_paths_select_user_root(const char* explicit_root, bool portable,
                                    const char* executable_path, const char* local_app_data,
                                    char* output, size_t output_size)
{
    const char* separator;
    if (explicit_root && explicit_root[0]) return storage_copy(explicit_root, output, output_size);
    if (portable) {
        const char* slash = executable_path ? strrchr(executable_path, '/') : NULL;
        const char* backslash = executable_path ? strrchr(executable_path, '\\') : NULL;
        separator = slash;
        if (!separator || (backslash && backslash > separator)) separator = backslash;
        if (!separator) return false;
        return storage_join(executable_path, (size_t)(separator - executable_path), "UserData", output, output_size);
    }
    if (!local_app_data || !local_app_data[0]) return false;
    return storage_join(local_app_data, strlen(local_app_data), "OpenJKDF2 AMD Enhanced", output, output_size);
}

static bool storage_is_value_option(const char* argument)
{
    return argument && (!strcmp(argument, "--data-dir") || !strcmp(argument, "--user-dir") ||
                        !strcmp(argument, "--diagnostics-dir"));
}

static bool storage_is_flag_option(const char* argument)
{
    return argument && (!strcmp(argument, "--portable") || !strcmp(argument, "--safe-mode") ||
                        !strcmp(argument, "--renderer-smoke-test"));
}

bool storage_paths_build_legacy_command(int argc, const char* const* argv,
                                        char* output, size_t output_size)
{
    size_t used = 0;
    if (argc < 0 || (argc > 0 && !argv) || !output || !output_size) return false;
    output[0] = '\0';
    for (int index = 1; index < argc; ++index) {
        const char* argument = argv[index];
        size_t length;
        if (!argument || !argument[0]) continue;
        if (storage_is_value_option(argument)) {
            if (++index >= argc) return false;
            continue;
        }
        if (storage_is_flag_option(argument)) continue;
        length = strlen(argument);
        if (used && used + 1 >= output_size) return false;
        if (used) output[used++] = ' ';
        if (used + length >= output_size) return false;
        memcpy(output + used, argument, length);
        used += length;
        output[used] = '\0';
    }
    return true;
}

bool storage_paths_build_crash_report_path(const char* diagnostics_root,
                                           char* output, size_t output_size)
{
    size_t root_length;
    if (!diagnostics_root || !diagnostics_root[0] || !output || !output_size)
        return false;
    root_length = strlen(diagnostics_root);
    return storage_join(diagnostics_root, root_length, "OpenJKDF2-crash.RPT", output, output_size);
}
