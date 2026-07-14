#include "General/ControlPreset.h"

#include <assert.h>
#include <string.h>

int main(void)
{
    assert(ControlPreset_Normalize(CONTROL_PRESET_CLASSIC) == CONTROL_PRESET_CLASSIC);
    assert(ControlPreset_Normalize(CONTROL_PRESET_MODERN) == CONTROL_PRESET_MODERN);
    assert(ControlPreset_Normalize(-1) == CONTROL_PRESET_MODERN);
    assert(ControlPreset_Normalize(99) == CONTROL_PRESET_MODERN);
    assert(strcmp(ControlPreset_Name(CONTROL_PRESET_CLASSIC), "Classic") == 0);
    assert(strcmp(ControlPreset_Name(CONTROL_PRESET_MODERN), "Modern") == 0);
    assert(strcmp(ControlPreset_Name(99), "Modern") == 0);
    return 0;
}
