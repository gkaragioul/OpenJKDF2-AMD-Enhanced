#include "General/ControlPreset.h"

ControlPreset ControlPreset_Normalize(int value)
{
    return value == CONTROL_PRESET_CLASSIC ? CONTROL_PRESET_CLASSIC : CONTROL_PRESET_MODERN;
}

const char* ControlPreset_Name(int value)
{
    return ControlPreset_Normalize(value) == CONTROL_PRESET_CLASSIC ? "Classic" : "Modern";
}
