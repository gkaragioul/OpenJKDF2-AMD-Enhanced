#include "General/DisplayMode.h"

#include <stdio.h>
#include <string.h>

static int failures;
#define CHECK(value) do { if (!(value)) { fprintf(stderr, "line %d: %s\n", __LINE__, #value); ++failures; } } while (0)

int main(void)
{
    CHECK(display_mode_from_config(0) == DISPLAY_MODE_WINDOWED);
    CHECK(display_mode_from_config(1) == DISPLAY_MODE_BORDERLESS);
    CHECK(display_mode_from_config(2) == DISPLAY_MODE_EXCLUSIVE);
    CHECK(display_mode_from_config(99) == DISPLAY_MODE_BORDERLESS);
    CHECK(strcmp(display_mode_name(DISPLAY_MODE_BORDERLESS), "borderless") == 0);
    CHECK(!display_mode_changes_desktop(DISPLAY_MODE_WINDOWED));
    CHECK(!display_mode_changes_desktop(DISPLAY_MODE_BORDERLESS));
    CHECK(display_mode_changes_desktop(DISPLAY_MODE_EXCLUSIVE));
    CHECK(display_mode_resolve(DISPLAY_MODE_EXCLUSIVE, false, false) == DISPLAY_MODE_BORDERLESS);
    CHECK(display_mode_resolve(DISPLAY_MODE_EXCLUSIVE, false, true) == DISPLAY_MODE_EXCLUSIVE);
    CHECK(display_mode_resolve(DISPLAY_MODE_BORDERLESS, true, true) == DISPLAY_MODE_WINDOWED);
    CHECK(display_resolution_valid(2560, 1440));
    CHECK(display_resolution_valid(3840, 2160));
    CHECK(!display_resolution_valid(0, 1440));
    CHECK(!display_resolution_valid(32768, 2160));
    return failures ? 1 : 0;
}
