#ifndef OPENJKDF2_DISPLAY_MODE_H
#define OPENJKDF2_DISPLAY_MODE_H

#include <stdbool.h>

typedef enum DisplayMode
{
    DISPLAY_MODE_WINDOWED = 0,
    DISPLAY_MODE_BORDERLESS = 1,
    DISPLAY_MODE_EXCLUSIVE = 2
} DisplayMode;

DisplayMode display_mode_from_config(int value);
DisplayMode display_mode_resolve(DisplayMode requested, bool safe_mode, bool restoration_guard_ready);
const char* display_mode_name(DisplayMode mode);
bool display_mode_changes_desktop(DisplayMode mode);
bool display_resolution_valid(int width, int height);

#endif
