#include "General/FrameRate.h"

#include <assert.h>

int main(void)
{
    static const int expected[] = {
        30, 40, 50, 60, 72, 90, 100, 120, 144, 165, 180, 200, 240,
        FRAME_RATE_DESKTOP_REFRESH, FRAME_RATE_UNLIMITED
    };
    size_t index;
    uint32_t missed = 99;

    assert(FrameRate_PresetCount() == sizeof(expected) / sizeof(expected[0]));
    for (index = 0; index < FrameRate_PresetCount(); ++index)
        assert(FrameRate_PresetValue(index) == expected[index]);
    assert(FrameRate_PresetValue(FrameRate_PresetCount()) == FRAME_RATE_UNLIMITED);

    assert(FrameRate_ResolveTarget(120, 165) == 120);
    assert(FrameRate_ResolveTarget(FRAME_RATE_DESKTOP_REFRESH, 165) == 165);
    assert(FrameRate_ResolveTarget(FRAME_RATE_DESKTOP_REFRESH, 0) == 60);
    assert(FrameRate_ResolveTarget(FRAME_RATE_UNLIMITED, 165) == 0);
    assert(FrameRate_ResolveTarget(-10, 165) == 0);

    assert(FrameRate_ValueFromSlider(0) == FRAME_RATE_UNLIMITED);
    assert(FrameRate_ValueFromSlider(120) == 120);
    assert(FrameRate_ValueFromSlider(FRAME_RATE_SLIDER_DESKTOP) == FRAME_RATE_DESKTOP_REFRESH);
    assert(FrameRate_ValueFromSlider(999) == FRAME_RATE_DESKTOP_REFRESH);
    assert(FrameRate_SliderFromValue(FRAME_RATE_UNLIMITED) == 0);
    assert(FrameRate_SliderFromValue(120) == 120);
    assert(FrameRate_SliderFromValue(FRAME_RATE_DESKTOP_REFRESH) == FRAME_RATE_SLIDER_DESKTOP);
    assert(FrameRate_SliderFromValue(999) == FRAME_RATE_NUMERIC_MAX);

    assert(!FrameRate_HasTimingCaution(30));
    assert(!FrameRate_HasTimingCaution(60));
    assert(!FrameRate_HasTimingCaution(100));
    assert(FrameRate_HasTimingCaution(120));
    assert(FrameRate_HasTimingCaution(240));
    assert(FrameRate_HasTimingCaution(FRAME_RATE_DESKTOP_REFRESH));
    assert(FrameRate_HasTimingCaution(FRAME_RATE_UNLIMITED));

    assert(FrameRate_PeriodNanoseconds(120) == 8333333ULL);
    assert(FrameRate_PeriodNanoseconds(60) == 16666667ULL);
    assert(FrameRate_PeriodNanoseconds(0) == 0ULL);

    assert(FrameRate_NextDeadline(1000, 1050, 100, &missed) == 1100);
    assert(missed == 0);
    assert(FrameRate_NextDeadline(1000, 1350, 100, &missed) == 1400);
    assert(missed == 3);
    assert(FrameRate_NextDeadline(0, 1350, 100, &missed) == 1450);
    assert(missed == 0);
    assert(FrameRate_NextDeadline(1000, 1350, 0, &missed) == 0);
    assert(missed == 0);

    return 0;
}
