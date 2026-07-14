#include "General/FrameTelemetry.h"

#include <string.h>

static uint64_t FrameTelemetry_previousTimestamp;
static double FrameTelemetry_samples[FRAME_TELEMETRY_SAMPLE_COUNT];
static unsigned int FrameTelemetry_nextSample;
static unsigned int FrameTelemetry_sampleCount;
static double FrameTelemetry_smoothedMilliseconds;

void FrameTelemetry_Reset(void)
{
    FrameTelemetry_previousTimestamp = 0;
    FrameTelemetry_nextSample = 0;
    FrameTelemetry_sampleCount = 0;
    FrameTelemetry_smoothedMilliseconds = 0.0;
    memset(FrameTelemetry_samples, 0, sizeof(FrameTelemetry_samples));
}

void FrameTelemetry_Record(uint64_t presentationTimestampNs)
{
    double milliseconds;

    if (!presentationTimestampNs)
        return;
    if (!FrameTelemetry_previousTimestamp)
    {
        FrameTelemetry_previousTimestamp = presentationTimestampNs;
        return;
    }
    if (presentationTimestampNs <= FrameTelemetry_previousTimestamp)
    {
        FrameTelemetry_previousTimestamp = presentationTimestampNs;
        return;
    }

    milliseconds = (double)(presentationTimestampNs - FrameTelemetry_previousTimestamp) / 1000000.0;
    FrameTelemetry_previousTimestamp = presentationTimestampNs;
    FrameTelemetry_samples[FrameTelemetry_nextSample] = milliseconds;
    FrameTelemetry_nextSample = (FrameTelemetry_nextSample + 1) % FRAME_TELEMETRY_SAMPLE_COUNT;
    if (FrameTelemetry_sampleCount < FRAME_TELEMETRY_SAMPLE_COUNT)
        ++FrameTelemetry_sampleCount;

    if (FrameTelemetry_smoothedMilliseconds == 0.0)
        FrameTelemetry_smoothedMilliseconds = milliseconds;
    else
        FrameTelemetry_smoothedMilliseconds = FrameTelemetry_smoothedMilliseconds * 0.9 + milliseconds * 0.1;
}

FrameTelemetrySnapshot FrameTelemetry_GetSnapshot(void)
{
    FrameTelemetrySnapshot snapshot;
    unsigned int oldest;
    unsigned int i;

    memset(&snapshot, 0, sizeof(snapshot));
    snapshot.sampleCount = FrameTelemetry_sampleCount;
    snapshot.frameMilliseconds = FrameTelemetry_smoothedMilliseconds;
    if (FrameTelemetry_smoothedMilliseconds > 0.0)
        snapshot.framesPerSecond = 1000.0 / FrameTelemetry_smoothedMilliseconds;

    oldest = FrameTelemetry_sampleCount == FRAME_TELEMETRY_SAMPLE_COUNT ? FrameTelemetry_nextSample : 0;
    for (i = 0; i < FrameTelemetry_sampleCount; ++i)
        snapshot.samples[i] = FrameTelemetry_samples[(oldest + i) % FRAME_TELEMETRY_SAMPLE_COUNT];
    return snapshot;
}

void FrameTelemetry_FormatGraph(const FrameTelemetrySnapshot* snapshot, double budgetMilliseconds,
                                char* output, size_t outputSize)
{
    static const char levels[] = ".:-=+*#@";
    unsigned int i;
    size_t count;

    if (!output || !outputSize)
        return;
    output[0] = '\0';
    if (!snapshot || budgetMilliseconds <= 0.0)
        return;

    count = snapshot->sampleCount;
    if (count >= outputSize)
        count = outputSize - 1;
    for (i = 0; i < count; ++i)
    {
        double ratio = snapshot->samples[i] / budgetMilliseconds;
        unsigned int level = ratio >= 2.0 ? 7u : (unsigned int)(ratio * 4.0);
        if (level > 7u)
            level = 7u;
        output[i] = levels[level];
    }
    output[count] = '\0';
}

int FrameTelemetry_IsUnstable(const FrameTelemetrySnapshot* snapshot, double budgetMilliseconds)
{
    unsigned int slowSamples = 0;
    unsigned int i;

    if (!snapshot || snapshot->sampleCount < 30 || budgetMilliseconds <= 0.0)
        return 0;
    for (i = 0; i < snapshot->sampleCount; ++i)
    {
        if (snapshot->samples[i] > budgetMilliseconds * 1.25)
            ++slowSamples;
    }
    return slowSamples * 5 > snapshot->sampleCount;
}
