#include "General/FrameTelemetry.h"

#include <assert.h>
#include <string.h>

int main(void)
{
    FrameTelemetrySnapshot snapshot;
    char graph[FRAME_TELEMETRY_SAMPLE_COUNT + 1];
    unsigned int i;

    FrameTelemetry_Reset();
    FrameTelemetry_Record(1000000000ULL);
    FrameTelemetry_Record(1016666667ULL);
    FrameTelemetry_Record(1033333334ULL);
    snapshot = FrameTelemetry_GetSnapshot();
    assert(snapshot.sampleCount == 2);
    assert(snapshot.framesPerSecond > 59.9 && snapshot.framesPerSecond < 60.1);
    assert(snapshot.frameMilliseconds > 16.6 && snapshot.frameMilliseconds < 16.8);

    for (i = 0; i < FRAME_TELEMETRY_SAMPLE_COUNT + 10; ++i)
        FrameTelemetry_Record(1050000001ULL + (uint64_t)i * 16666667ULL);
    snapshot = FrameTelemetry_GetSnapshot();
    assert(snapshot.sampleCount == FRAME_TELEMETRY_SAMPLE_COUNT);

    FrameTelemetry_FormatGraph(&snapshot, 16.666667, graph, sizeof(graph));
    assert(strlen(graph) == FRAME_TELEMETRY_SAMPLE_COUNT);
    assert(strspn(graph, ".:-=+*#@") == FRAME_TELEMETRY_SAMPLE_COUNT);
    assert(!FrameTelemetry_IsUnstable(&snapshot, 16.666667));

    FrameTelemetry_Reset();
    FrameTelemetry_Record(1000000000ULL);
    for (i = 1; i <= 30; ++i)
        FrameTelemetry_Record(1000000000ULL + (uint64_t)i * 25000000ULL);
    snapshot = FrameTelemetry_GetSnapshot();
    assert(FrameTelemetry_IsUnstable(&snapshot, 16.666667));
    return 0;
}
