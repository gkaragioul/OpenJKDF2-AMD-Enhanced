#include "General/MouseSmoothing.h"

int MouseSmoothing_Filter(int currentDelta, int previousDelta)
{
    int weighted = currentDelta * 13 + previousDelta * 7;
    return (weighted + (weighted >= 0 ? 10 : -10)) / 20;
}
