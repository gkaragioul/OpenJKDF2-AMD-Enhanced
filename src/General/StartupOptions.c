#include "General/StartupOptions.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static bool startup_equal_ignore_case(const char* left, const char* right)
{
    if (!left || !right) return false;
    while (*left && *right) {
        if (tolower((unsigned char)*left) != tolower((unsigned char)*right)) return false;
        ++left;
        ++right;
    }
    return *left == *right;
}

static bool startup_copy_value(char* destination, size_t capacity, const char* value, const char* option, char* error)
{
    size_t length = value ? strlen(value) : 0;
    if (!length) {
        snprintf(error, STARTUP_ERROR_CAPACITY, "%s requires a non-empty value", option);
        return false;
    }
    if (length >= capacity) {
        snprintf(error, STARTUP_ERROR_CAPACITY, "%s value is too long", option);
        return false;
    }
    memcpy(destination, value, length + 1);
    return true;
}

StartupOptions startup_options_parse(int argc, const char* const* argv)
{
    StartupOptions options = { 0 };
    int index;
    if (argc < 0 || (argc > 0 && !argv)) {
        strcpy(options.error, "invalid argument vector");
        return options;
    }
    for (index = 1; index < argc; ++index) {
        const char* argument = argv[index];
        if (!argument) continue;
        if (strcmp(argument, "--safe-mode") == 0) {
            options.safe_mode = true;
            options.force_windowed = true;
            options.conservative_renderer = true;
            options.frame_limit = 60;
        } else if (strcmp(argument, "--renderer-smoke-test") == 0) {
            options.renderer_smoke_test = true;
        } else if (strcmp(argument, "--portable") == 0) {
            options.portable = true;
        } else if (strcmp(argument, "--diagnostics-dir") == 0 || strcmp(argument, "--data-dir") == 0 ||
                   strcmp(argument, "--user-dir") == 0 ||
                   startup_equal_ignore_case(argument, "-path") || startup_equal_ignore_case(argument, "/path")) {
            char* destination;
            const char* option_name;
            if (index + 1 >= argc) {
                snprintf(options.error, sizeof(options.error), "%s requires a value", argument);
                return options;
            }
            option_name = argument;
            if (strcmp(argument, "--diagnostics-dir") == 0) destination = options.diagnostics_dir;
            else if (strcmp(argument, "--user-dir") == 0) destination = options.user_dir;
            else destination = options.data_dir;
            if (!startup_copy_value(destination, STARTUP_PATH_CAPACITY, argv[++index], option_name, options.error)) {
                return options;
            }
        }
    }
    return options;
}
