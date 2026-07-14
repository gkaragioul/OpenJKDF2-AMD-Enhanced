#ifndef OPENJKDF2_CONTROL_PRESET_H
#define OPENJKDF2_CONTROL_PRESET_H

typedef enum ControlPreset
{
    CONTROL_PRESET_CLASSIC = 0,
    CONTROL_PRESET_MODERN = 1
} ControlPreset;

ControlPreset ControlPreset_Normalize(int value);
const char* ControlPreset_Name(int value);

#endif
