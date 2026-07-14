#include "General/DisplaySelection.h"

static int clamp(int value, int minimum, int maximum)
{
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

static int exact_mode_available(const DisplayMonitor* monitor, DisplaySettings settings)
{
    int index;
    for (index = 0; index < monitor->mode_count && index < DISPLAY_SELECTION_MAX_MODES; ++index)
    {
        const DisplayResolution* mode = &monitor->modes[index];
        if (mode->width == settings.width &&
            mode->height == settings.height &&
            mode->refresh_hz == settings.refresh_hz)
            return 1;
    }
    return 0;
}

DisplaySelectionResult display_selection_resolve(
    const DisplayInventory* inventory,
    DisplaySettings requested,
    int restoration_guard_ready)
{
    DisplaySelectionResult result = {0};
    const DisplayMonitor* monitor;

    result.settings = requested;
    result.reason = DISPLAY_SELECTION_OK;

    if (!inventory || inventory->monitor_count <= 0)
    {
        result.reason = DISPLAY_SELECTION_NO_DISPLAYS;
        return result;
    }

    if (result.settings.monitor < 0 || result.settings.monitor >= inventory->monitor_count)
    {
        int primary = inventory->primary_monitor;
        if (primary < 0 || primary >= inventory->monitor_count)
            primary = 0;
        result.settings.monitor = primary;
        result.normalized = 1;
        result.reason = DISPLAY_SELECTION_MONITOR_FALLBACK;
    }

    monitor = &inventory->monitors[result.settings.monitor];
    switch (result.settings.mode)
    {
        case DISPLAY_MODE_WINDOWED:
        {
            int width = clamp(result.settings.width,
                              DISPLAY_SELECTION_MIN_WIDTH,
                              DISPLAY_SELECTION_MAX_DIMENSION);
            int height = clamp(result.settings.height,
                               DISPLAY_SELECTION_MIN_HEIGHT,
                               DISPLAY_SELECTION_MAX_DIMENSION);
            if (width != result.settings.width || height != result.settings.height)
            {
                result.settings.width = width;
                result.settings.height = height;
                result.normalized = 1;
                result.reason = DISPLAY_SELECTION_WINDOW_SIZE_CLAMPED;
            }
            if (result.settings.refresh_hz != 0)
            {
                result.settings.refresh_hz = 0;
                result.normalized = 1;
                if (result.reason == DISPLAY_SELECTION_OK)
                    result.reason = DISPLAY_SELECTION_COMPOSITOR_REFRESH;
            }
            result.accepted = 1;
            return result;
        }

        case DISPLAY_MODE_BORDERLESS:
            if (monitor->desktop_width <= 0 || monitor->desktop_height <= 0)
            {
                result.reason = DISPLAY_SELECTION_NO_DISPLAYS;
                return result;
            }
            if (result.settings.width != monitor->desktop_width ||
                result.settings.height != monitor->desktop_height ||
                result.settings.refresh_hz != monitor->desktop_refresh_hz)
            {
                result.normalized = 1;
                result.reason = DISPLAY_SELECTION_DESKTOP_MODE;
            }
            result.settings.width = monitor->desktop_width;
            result.settings.height = monitor->desktop_height;
            result.settings.refresh_hz = monitor->desktop_refresh_hz;
            result.accepted = 1;
            return result;

        case DISPLAY_MODE_EXCLUSIVE:
            if (!restoration_guard_ready)
            {
                result.reason = DISPLAY_SELECTION_EXCLUSIVE_GUARD_UNAVAILABLE;
                return result;
            }
            if (!exact_mode_available(monitor, result.settings))
            {
                result.reason = DISPLAY_SELECTION_EXCLUSIVE_MODE_UNAVAILABLE;
                return result;
            }
            result.accepted = 1;
            return result;

        default:
            result.reason = DISPLAY_SELECTION_INVALID_MODE;
            return result;
    }
}
