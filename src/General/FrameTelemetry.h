#ifndef OPENJKDF2_FRAME_TELEMETRY_H
#define OPENJKDF2_FRAME_TELEMETRY_H

#include <stddef.h>
#include <stdint.h>

#define FRAME_TELEMETRY_SAMPLE_COUNT 60

typedef struct FrameTelemetrySnapshot
{
    double framesPerSecond;
    double frameMilliseconds;
    double samples[FRAME_TELEMETRY_SAMPLE_COUNT];
    unsigned int sampleCount;
} FrameTelemetrySnapshot;

void FrameTelemetry_Reset(void);
void FrameTelemetry_Record(uint64_t presentationTimestampNs);
FrameTelemetrySnapshot FrameTelemetry_GetSnapshot(void);
void FrameTelemetry_FormatGraph(const FrameTelemetrySnapshot* snapshot, double budgetMilliseconds,
                                char* output, size_t outputSize);
int FrameTelemetry_IsUnstable(const FrameTelemetrySnapshot* snapshot, double budgetMilliseconds);

#endif
