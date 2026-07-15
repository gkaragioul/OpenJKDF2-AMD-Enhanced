#include "General/DefaultSettingsMigration.h"

#include "General/ControlPreset.h"

int default_settings_should_migrate_controls(int stored_version, int preset, int exact_stock_classic)
{
    return stored_version < CONTROL_DEFAULTS_VERSION &&
        preset == CONTROL_PRESET_CLASSIC &&
        exact_stock_classic;
}

DisplayMode default_settings_migrate_display(
    int stored_version,
    DisplayMode stored_mode,
    int width,
    int height,
    int stock_width,
    int stock_height,
    int* changed)
{
    const int should_migrate = stored_version < WINDOW_DEFAULTS_VERSION &&
        stored_mode == DISPLAY_MODE_WINDOWED &&
        width == stock_width &&
        height == stock_height;

    if (changed) {
        *changed = should_migrate;
    }

    return should_migrate ? DISPLAY_MODE_BORDERLESS : stored_mode;
}
