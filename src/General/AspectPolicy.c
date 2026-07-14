#include "General/AspectPolicy.h"

ResolutionLayoutRect AspectPolicy_Destination(
    int outputWidth,
    int outputHeight,
    int contentWidth,
    int contentHeight,
    int preserveAspect)
{
    ResolutionLayoutRect result = {0.0, 0.0, 0.0, 0.0};

    if (outputWidth <= 0 || outputHeight <= 0 || contentWidth <= 0 || contentHeight <= 0)
        return result;
    if (!preserveAspect)
    {
        result.width = (double)outputWidth;
        result.height = (double)outputHeight;
        return result;
    }
    return ResolutionLayout_FitAspect(
        outputWidth,
        outputHeight,
        (double)contentWidth / (double)contentHeight);
}
