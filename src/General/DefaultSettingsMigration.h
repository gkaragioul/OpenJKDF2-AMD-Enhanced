#ifndef OPENJKDF2_DEFAULT_SETTINGS_MIGRATION_H
#define OPENJKDF2_DEFAULT_SETTINGS_MIGRATION_H

#include "General/DisplayMode.h"

#define CONTROL_DEFAULTS_VERSION 1
#define WINDOW_DEFAULTS_VERSION 1

int default_settings_should_migrate_controls(int stored_version, int preset, int exact_stock_classic);
DisplayMode default_settings_migrate_display(
    int stored_version,
    DisplayMode stored_mode,
    int width,
    int height,
    int stock_width,
    int stock_height,
    int* changed);

#endif
