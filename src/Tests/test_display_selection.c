#include "General/DisplaySelection.h"

#include <stdio.h>
#include <string.h>

static int failures;
#define CHECK(value) do { if (!(value)) { fprintf(stderr, "line %d: %s\n", __LINE__, #value); ++failures; } } while (0)

static DisplaySettings settings(DisplayMode mode, int monitor, int width, int height, int refresh)
{
    DisplaySettings value = {mode, monitor, width, height, refresh, 0};
    return value;
}

static DisplayInventory inventory(void)
{
    DisplayInventory value;
    memset(&value, 0, sizeof(value));
    value.monitor_count = 2;
    value.primary_monitor = 0;
    value.monitors[0].desktop_width = 2560;
    value.monitors[0].desktop_height = 1440;
    value.monitors[0].desktop_refresh_hz = 165;
    value.monitors[0].mode_count = 2;
    value.monitors[0].modes[0] = (DisplayResolution){1920, 1080, 120};
    value.monitors[0].modes[1] = (DisplayResolution){2560, 1440, 165};
    value.monitors[1].desktop_width = 1920;
    value.monitors[1].desktop_height = 1080;
    value.monitors[1].desktop_refresh_hz = 60;
    value.monitors[1].mode_count = 1;
    value.monitors[1].modes[0] = (DisplayResolution){1920, 1080, 60};
    return value;
}

static void test_invalid_monitor_falls_back_to_primary(void)
{
    DisplayInventory displays = inventory();
    DisplaySelectionResult result = display_selection_resolve(&displays, settings(DISPLAY_MODE_WINDOWED, 7, 1280, 720, 0), 0);
    CHECK(result.accepted);
    CHECK(result.normalized);
    CHECK(result.reason == DISPLAY_SELECTION_MONITOR_FALLBACK);
    CHECK(result.settings.monitor == 0);
}

static void test_windowed_dimensions_clamp_and_refresh_clears(void)
{
    DisplayInventory displays = inventory();
    DisplaySelectionResult small = display_selection_resolve(&displays, settings(DISPLAY_MODE_WINDOWED, 0, 100, 200, 144), 0);
    DisplaySelectionResult large = display_selection_resolve(&displays, settings(DISPLAY_MODE_WINDOWED, 0, 99999, 99999, 165), 0);
    CHECK(small.accepted && small.settings.width == 640 && small.settings.height == 480);
    CHECK(small.settings.refresh_hz == 0);
    CHECK(small.reason == DISPLAY_SELECTION_WINDOW_SIZE_CLAMPED);
    CHECK(large.accepted);
    CHECK(large.settings.width == DISPLAY_SELECTION_MAX_DIMENSION);
    CHECK(large.settings.height == DISPLAY_SELECTION_MAX_DIMENSION);
    CHECK(large.settings.refresh_hz == 0);
}

static void test_borderless_uses_selected_desktop(void)
{
    DisplayInventory displays = inventory();
    DisplaySelectionResult result = display_selection_resolve(&displays, settings(DISPLAY_MODE_BORDERLESS, 1, 800, 600, 144), 0);
    CHECK(result.accepted);
    CHECK(result.settings.monitor == 1);
    CHECK(result.settings.width == 1920);
    CHECK(result.settings.height == 1080);
    CHECK(result.settings.refresh_hz == 60);
    CHECK(result.reason == DISPLAY_SELECTION_DESKTOP_MODE);
}

static void test_exclusive_requires_guard_and_exact_mode(void)
{
    DisplayInventory displays = inventory();
    DisplaySelectionResult guarded = display_selection_resolve(&displays, settings(DISPLAY_MODE_EXCLUSIVE, 0, 1920, 1080, 120), 0);
    DisplaySelectionResult unsupported = display_selection_resolve(&displays, settings(DISPLAY_MODE_EXCLUSIVE, 0, 1920, 1080, 144), 1);
    DisplaySelectionResult valid = display_selection_resolve(&displays, settings(DISPLAY_MODE_EXCLUSIVE, 0, 1920, 1080, 120), 1);
    CHECK(!guarded.accepted);
    CHECK(guarded.reason == DISPLAY_SELECTION_EXCLUSIVE_GUARD_UNAVAILABLE);
    CHECK(!unsupported.accepted);
    CHECK(unsupported.reason == DISPLAY_SELECTION_EXCLUSIVE_MODE_UNAVAILABLE);
    CHECK(valid.accepted);
    CHECK(!valid.normalized);
    CHECK(valid.reason == DISPLAY_SELECTION_OK);
}

static void test_missing_inventory_rejects_safely(void)
{
    DisplayInventory displays;
    memset(&displays, 0, sizeof(displays));
    DisplaySelectionResult result = display_selection_resolve(&displays, settings(DISPLAY_MODE_BORDERLESS, 0, 1920, 1080, 60), 0);
    CHECK(!result.accepted);
    CHECK(result.reason == DISPLAY_SELECTION_NO_DISPLAYS);
}

int main(void)
{
    test_invalid_monitor_falls_back_to_primary();
    test_windowed_dimensions_clamp_and_refresh_clears();
    test_borderless_uses_selected_desktop();
    test_exclusive_requires_guard_and_exact_mode();
    test_missing_inventory_rejects_safely();
    return failures ? 1 : 0;
}
