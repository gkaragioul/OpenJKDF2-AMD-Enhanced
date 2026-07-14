#ifndef OPENJKDF2_STARTUP_OPTIONS_H
#define OPENJKDF2_STARTUP_OPTIONS_H

#include <stdbool.h>

#define STARTUP_PATH_CAPACITY 512
#define STARTUP_ERROR_CAPACITY 160

typedef struct StartupOptions
{
    bool safe_mode;
    bool renderer_smoke_test;
    bool portable;
    bool force_windowed;
    bool conservative_renderer;
    int frame_limit;
    char diagnostics_dir[STARTUP_PATH_CAPACITY];
    char data_dir[STARTUP_PATH_CAPACITY];
    char user_dir[STARTUP_PATH_CAPACITY];
    char error[STARTUP_ERROR_CAPACITY];
} StartupOptions;

StartupOptions startup_options_parse(int argc, const char* const* argv);

#endif
