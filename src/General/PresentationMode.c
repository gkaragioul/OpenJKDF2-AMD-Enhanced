#include "General/PresentationMode.h"

PresentationVsyncMode PresentationMode_NormalizeVsync(int value)
{
    if (value == PRESENTATION_VSYNC_ADAPTIVE)
        return PRESENTATION_VSYNC_ADAPTIVE;
    if (value == PRESENTATION_VSYNC_OFF)
        return PRESENTATION_VSYNC_OFF;
    return PRESENTATION_VSYNC_ON;
}

PresentationVsyncMode PresentationMode_VsyncFromSlider(int sliderPosition)
{
    if (sliderPosition <= 0)
        return PRESENTATION_VSYNC_OFF;
    if (sliderPosition == 1)
        return PRESENTATION_VSYNC_ON;
    return PRESENTATION_VSYNC_ADAPTIVE;
}

int PresentationMode_SliderFromVsync(int value)
{
    PresentationVsyncMode mode = PresentationMode_NormalizeVsync(value);
    if (mode == PRESENTATION_VSYNC_ADAPTIVE)
        return 2;
    return mode == PRESENTATION_VSYNC_ON ? 1 : 0;
}

const char* PresentationMode_VsyncName(int value)
{
    PresentationVsyncMode mode = PresentationMode_NormalizeVsync(value);
    if (mode == PRESENTATION_VSYNC_ADAPTIVE)
        return "Adaptive";
    return mode == PRESENTATION_VSYNC_ON ? "On" : "Off";
}
