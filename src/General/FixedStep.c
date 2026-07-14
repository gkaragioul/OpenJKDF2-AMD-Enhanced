#include "General/FixedStep.h"

#include <math.h>

uint32_t FixedStep_Consume(double elapsedSeconds, double stepSeconds, uint32_t maximumSteps, double* accumulatorSeconds)
{
    double total;
    uint64_t availableSteps;
    uint32_t consumedSteps;

    if (!accumulatorSeconds || elapsedSeconds <= 0.0 || stepSeconds <= 0.0 || maximumSteps == 0)
        return 0;

    total = *accumulatorSeconds + elapsedSeconds;
    if (total < 0.0)
        total = 0.0;

    availableSteps = (uint64_t)((total + stepSeconds * 0.000000001) / stepSeconds);
    consumedSteps = availableSteps > maximumSteps ? maximumSteps : (uint32_t)availableSteps;

    if (availableSteps > maximumSteps)
        *accumulatorSeconds = fmod(total, stepSeconds);
    else
        *accumulatorSeconds = total - (double)consumedSteps * stepSeconds;

    if (*accumulatorSeconds < 0.0)
        *accumulatorSeconds = 0.0;
    if (*accumulatorSeconds >= stepSeconds)
        *accumulatorSeconds = fmod(*accumulatorSeconds, stepSeconds);
    return consumedSteps;
}
