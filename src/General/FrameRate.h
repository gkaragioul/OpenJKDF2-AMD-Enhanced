#ifndef OPENJKDF2_FRAME_RATE_H
#define OPENJKDF2_FRAME_RATE_H

#include <stddef.h>
#include <stdint.h>

#define FRAME_RATE_DESKTOP_REFRESH (-1)
#define FRAME_RATE_UNLIMITED (0)
#define FRAME_RATE_NUMERIC_MAX (360)
#define FRAME_RATE_SLIDER_DESKTOP (361)

size_t FrameRate_PresetCount(void);
int FrameRate_PresetValue(size_t index);
int FrameRate_ResolveTarget(int configuredRate, int desktopRefreshRate);
uint64_t FrameRate_PeriodNanoseconds(int targetRate);
uint64_t FrameRate_NextDeadline(uint64_t previousDeadline, uint64_t now, uint64_t period, uint32_t* missedDeadlines);
int FrameRate_ValueFromSlider(int sliderPosition);
int FrameRate_SliderFromValue(int configuredRate);

#endif
