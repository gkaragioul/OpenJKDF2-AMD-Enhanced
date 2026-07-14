#include "General/DisplayMode.h"

DisplayMode display_mode_from_config(int value)
{
    switch (value) {
        case DISPLAY_MODE_WINDOWED: return DISPLAY_MODE_WINDOWED;
        case DISPLAY_MODE_BORDERLESS: return DISPLAY_MODE_BORDERLESS;
        case DISPLAY_MODE_EXCLUSIVE: return DISPLAY_MODE_EXCLUSIVE;
        default: return DISPLAY_MODE_BORDERLESS;
    }
}

DisplayMode display_mode_resolve(DisplayMode requested, bool safe_mode, bool restoration_guard_ready)
{
    if (safe_mode) return DISPLAY_MODE_WINDOWED;
    if (requested == DISPLAY_MODE_EXCLUSIVE && !restoration_guard_ready) return DISPLAY_MODE_BORDERLESS;
    return display_mode_from_config((int)requested);
}

const char* display_mode_name(DisplayMode mode)
{
    switch (mode) {
        case DISPLAY_MODE_WINDOWED: return "windowed";
        case DISPLAY_MODE_BORDERLESS: return "borderless";
        case DISPLAY_MODE_EXCLUSIVE: return "exclusive";
        default: return "borderless";
    }
}

bool display_mode_changes_desktop(DisplayMode mode)
{
    return mode == DISPLAY_MODE_EXCLUSIVE;
}

bool display_resolution_valid(int width, int height)
{
    return width >= 320 && height >= 200 && width <= 16384 && height <= 16384;
}
