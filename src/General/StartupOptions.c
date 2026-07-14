#include "General/StartupOptions.h"

#include <stdio.h>
#include <string.h>

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
        const char* validation_prefix = "--validation-observer=";
        const size_t validation_prefix_length = strlen(validation_prefix);
        const char* frame_limit_prefix = "--frame-limit=";
        const size_t frame_limit_prefix_length = strlen(frame_limit_prefix);
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
        } else if (strncmp(argument, validation_prefix, validation_prefix_length) == 0 ||
                   strcmp(argument, "--validation-observer") == 0) {
            const char* value;
            if (strcmp(argument, "--validation-observer") == 0) {
                if (index + 1 >= argc) {
                    strcpy(options.error, "--validation-observer requires a value");
                    return options;
                }
                value = argv[++index];
            } else {
                value = argument + validation_prefix_length;
            }
            if (!value || strcmp(value, "timing-domains") != 0) {
                strcpy(options.error, "--validation-observer must be timing-domains");
                return options;
            }
            options.validation_observer = STARTUP_VALIDATION_TIMING_DOMAINS;
        } else if (strncmp(argument, frame_limit_prefix, frame_limit_prefix_length) == 0 ||
                   strcmp(argument, "--frame-limit") == 0) {
            const char* value;
            if (strcmp(argument, "--frame-limit") == 0) {
                if (index + 1 >= argc) {
                    strcpy(options.error, "--frame-limit requires a value");
                    return options;
                }
                value = argv[++index];
            } else {
                value = argument + frame_limit_prefix_length;
            }
            if (!value || (strcmp(value, "60") != 0 && strcmp(value, "120") != 0)) {
                strcpy(options.error, "--frame-limit must be 60 or 120");
                return options;
            }
            options.frame_limit = strcmp(value, "60") == 0 ? 60 : 120;
        } else if (strcmp(argument, "--diagnostics-dir") == 0 || strcmp(argument, "--data-dir") == 0 ||
                   strcmp(argument, "--user-dir") == 0) {
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
