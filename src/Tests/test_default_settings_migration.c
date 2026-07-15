#include "General/ControlPreset.h"
#include "General/DefaultSettingsMigration.h"
#include "General/DisplayMode.h"

#include <assert.h>

int main(void)
{
    int changed = 0;

    assert(default_settings_should_migrate_controls(0, CONTROL_PRESET_CLASSIC, 1));
    assert(!default_settings_should_migrate_controls(0, CONTROL_PRESET_CLASSIC, 0));
    assert(!default_settings_should_migrate_controls(1, CONTROL_PRESET_CLASSIC, 1));
    assert(!default_settings_should_migrate_controls(0, CONTROL_PRESET_MODERN, 1));

    assert(default_settings_migrate_display(0, DISPLAY_MODE_WINDOWED,
        1280, 960, 1280, 960, &changed) == DISPLAY_MODE_BORDERLESS);
    assert(changed == 1);

    changed = 0;
    assert(default_settings_migrate_display(0, DISPLAY_MODE_WINDOWED,
        1600, 900, 1280, 960, &changed) == DISPLAY_MODE_WINDOWED);
    assert(changed == 0);

    changed = 1;
    assert(default_settings_migrate_display(1, DISPLAY_MODE_WINDOWED,
        1280, 960, 1280, 960, &changed) == DISPLAY_MODE_WINDOWED);
    assert(changed == 0);

    return 0;
}
