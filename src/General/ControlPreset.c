#include "General/ControlPreset.h"

ControlPreset ControlPreset_Default(void)
{
    return CONTROL_PRESET_MODERN;
}

ControlPreset ControlPreset_Normalize(int value)
{
    return value == CONTROL_PRESET_CLASSIC ? CONTROL_PRESET_CLASSIC : ControlPreset_Default();
}

const char* ControlPreset_Name(int value)
{
    return ControlPreset_Normalize(value) == CONTROL_PRESET_CLASSIC ? "Classic" : "Modern";
}
