#ifndef OPENJKDF2_FIXED_STEP_H
#define OPENJKDF2_FIXED_STEP_H

#include <stdint.h>

uint32_t FixedStep_Consume(double elapsedSeconds, double stepSeconds, uint32_t maximumSteps, double* accumulatorSeconds);

#endif
