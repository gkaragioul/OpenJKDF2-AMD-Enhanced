#include "General/FrameRate.h"

static const int FrameRate_presets[] = {
    30, 40, 50, 60, 72, 90, 100, 120, 144, 165, 180, 200, 240,
    FRAME_RATE_DESKTOP_REFRESH, FRAME_RATE_UNLIMITED
};

size_t FrameRate_PresetCount(void)
{
    return sizeof(FrameRate_presets) / sizeof(FrameRate_presets[0]);
}

int FrameRate_PresetValue(size_t index)
{
    if (index >= FrameRate_PresetCount())
        return FRAME_RATE_UNLIMITED;
    return FrameRate_presets[index];
}

int FrameRate_ResolveTarget(int configuredRate, int desktopRefreshRate)
{
    if (configuredRate == FRAME_RATE_DESKTOP_REFRESH)
        return desktopRefreshRate > 0 ? desktopRefreshRate : 60;
    if (configuredRate > 0)
        return configuredRate;
    return FRAME_RATE_UNLIMITED;
}

uint64_t FrameRate_PeriodNanoseconds(int targetRate)
{
    if (targetRate <= 0)
        return 0;
    return (1000000000ULL + (uint64_t)targetRate / 2ULL) / (uint64_t)targetRate;
}

uint64_t FrameRate_NextDeadline(uint64_t previousDeadline, uint64_t now, uint64_t period, uint32_t* missedDeadlines)
{
    uint64_t next;
    uint64_t missed;

    if (missedDeadlines)
        *missedDeadlines = 0;
    if (period == 0)
        return 0;
    if (previousDeadline == 0)
        return now + period;

    next = previousDeadline + period;
    if (next > now)
        return next;

    missed = ((now - next) / period) + 1ULL;
    if (missedDeadlines)
        *missedDeadlines = missed > UINT32_MAX ? UINT32_MAX : (uint32_t)missed;
    return next + missed * period;
}

int FrameRate_ValueFromSlider(int sliderPosition)
{
    if (sliderPosition <= 0)
        return FRAME_RATE_UNLIMITED;
    if (sliderPosition >= FRAME_RATE_SLIDER_DESKTOP)
        return FRAME_RATE_DESKTOP_REFRESH;
    return sliderPosition;
}

int FrameRate_SliderFromValue(int configuredRate)
{
    if (configuredRate == FRAME_RATE_DESKTOP_REFRESH)
        return FRAME_RATE_SLIDER_DESKTOP;
    if (configuredRate <= 0)
        return 0;
    if (configuredRate > FRAME_RATE_NUMERIC_MAX)
        return FRAME_RATE_NUMERIC_MAX;
    return configuredRate;
}

int FrameRate_HasTimingCaution(int configuredRate)
{
    return configuredRate <= FRAME_RATE_UNLIMITED || configuredRate > 100;
}
