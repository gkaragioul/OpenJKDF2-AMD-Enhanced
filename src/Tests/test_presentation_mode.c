#include "General/PresentationMode.h"

#include <assert.h>
#include <string.h>

int main(void)
{
    assert(PresentationMode_NormalizeVsync(-1) == PRESENTATION_VSYNC_ADAPTIVE);
    assert(PresentationMode_NormalizeVsync(0) == PRESENTATION_VSYNC_OFF);
    assert(PresentationMode_NormalizeVsync(1) == PRESENTATION_VSYNC_ON);
    assert(PresentationMode_NormalizeVsync(99) == PRESENTATION_VSYNC_ON);

    assert(PresentationMode_VsyncFromSlider(0) == PRESENTATION_VSYNC_OFF);
    assert(PresentationMode_VsyncFromSlider(1) == PRESENTATION_VSYNC_ON);
    assert(PresentationMode_VsyncFromSlider(2) == PRESENTATION_VSYNC_ADAPTIVE);
    assert(PresentationMode_VsyncFromSlider(99) == PRESENTATION_VSYNC_ADAPTIVE);
    assert(PresentationMode_SliderFromVsync(PRESENTATION_VSYNC_OFF) == 0);
    assert(PresentationMode_SliderFromVsync(PRESENTATION_VSYNC_ON) == 1);
    assert(PresentationMode_SliderFromVsync(PRESENTATION_VSYNC_ADAPTIVE) == 2);

    assert(strcmp(PresentationMode_VsyncName(PRESENTATION_VSYNC_OFF), "Off") == 0);
    assert(strcmp(PresentationMode_VsyncName(PRESENTATION_VSYNC_ON), "On") == 0);
    assert(strcmp(PresentationMode_VsyncName(PRESENTATION_VSYNC_ADAPTIVE), "Adaptive") == 0);
    return 0;
}
