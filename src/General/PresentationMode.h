#ifndef OPENJKDF2_PRESENTATION_MODE_H
#define OPENJKDF2_PRESENTATION_MODE_H

typedef enum PresentationVsyncMode
{
    PRESENTATION_VSYNC_ADAPTIVE = -1,
    PRESENTATION_VSYNC_OFF = 0,
    PRESENTATION_VSYNC_ON = 1
} PresentationVsyncMode;

PresentationVsyncMode PresentationMode_NormalizeVsync(int value);
PresentationVsyncMode PresentationMode_VsyncFromSlider(int sliderPosition);
int PresentationMode_SliderFromVsync(int value);
const char* PresentationMode_VsyncName(int value);

#endif
