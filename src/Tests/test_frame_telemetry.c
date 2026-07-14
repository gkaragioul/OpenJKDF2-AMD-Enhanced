#include "General/FrameTelemetry.h"

#include <assert.h>
#include <string.h>

int main(void)
{
    FrameTelemetrySnapshot snapshot;
    FrameTelemetryStatistics statistics;
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
    assert(FrameTelemetry_CalculateStatistics(&snapshot, &statistics));
    assert(statistics.sampleCount == FRAME_TELEMETRY_SAMPLE_COUNT);
    assert(statistics.medianMilliseconds > 16.6 && statistics.medianMilliseconds < 16.8);
    assert(statistics.p95Milliseconds > 16.6 && statistics.p95Milliseconds < 16.8);
    assert(statistics.p99Milliseconds > 16.6 && statistics.p99Milliseconds < 16.8);
    assert(statistics.worstMilliseconds > 16.6 && statistics.worstMilliseconds < 16.8);

    memset(&snapshot, 0, sizeof(snapshot));
    snapshot.sampleCount = 5;
    snapshot.samples[0] = 5.0;
    snapshot.samples[1] = 1.0;
    snapshot.samples[2] = 3.0;
    snapshot.samples[3] = 2.0;
    snapshot.samples[4] = 4.0;
    assert(FrameTelemetry_CalculateStatistics(&snapshot, &statistics));
    assert(statistics.medianMilliseconds == 3.0);
    assert(statistics.p95Milliseconds == 5.0);
    assert(statistics.p99Milliseconds == 5.0);
    assert(statistics.worstMilliseconds == 5.0);
    assert(!FrameTelemetry_CalculateStatistics(NULL, &statistics));
    assert(!FrameTelemetry_CalculateStatistics(&snapshot, NULL));
    snapshot.sampleCount = 0;
    assert(!FrameTelemetry_CalculateStatistics(&snapshot, &statistics));

    FrameTelemetry_Reset();
    FrameTelemetry_Record(1000000000ULL);
    for (i = 1; i <= 30; ++i)
        FrameTelemetry_Record(1000000000ULL + (uint64_t)i * 25000000ULL);
    snapshot = FrameTelemetry_GetSnapshot();
    assert(FrameTelemetry_IsUnstable(&snapshot, 16.666667));
    return 0;
}
