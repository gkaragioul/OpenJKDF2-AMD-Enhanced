#include "General/ResolutionLayout.h"

static double ResolutionLayout_Clamp(double value, double minimum, double maximum)
{
    if (value < minimum)
        return minimum;
    if (value > maximum)
        return maximum;
    return value;
}

ResolutionLayoutRect ResolutionLayout_FitAspect(int outputWidth, int outputHeight, double contentAspect)
{
    ResolutionLayoutRect result = {0.0, 0.0, 0.0, 0.0};
    double outputAspect;

    if (outputWidth <= 0 || outputHeight <= 0 || contentAspect <= 0.0)
        return result;

    outputAspect = (double)outputWidth / (double)outputHeight;
    if (outputAspect > contentAspect)
    {
        result.height = (double)outputHeight;
        result.width = result.height * contentAspect;
        result.x = ((double)outputWidth - result.width) * 0.5;
    }
    else
    {
        result.width = (double)outputWidth;
        result.height = result.width / contentAspect;
        result.y = ((double)outputHeight - result.height) * 0.5;
    }

    return result;
}

void ResolutionLayout_MapPoint(
    const ResolutionLayoutRect* viewport,
    double x,
    double y,
    double logicalWidth,
    double logicalHeight,
    int clampToBounds,
    double* logicalX,
    double* logicalY)
{
    double mappedX = 0.0;
    double mappedY = 0.0;

    if (viewport && viewport->width > 0.0 && viewport->height > 0.0 && logicalWidth > 0.0 && logicalHeight > 0.0)
    {
        mappedX = ((x - viewport->x) / viewport->width) * logicalWidth;
        mappedY = ((y - viewport->y) / viewport->height) * logicalHeight;
        if (clampToBounds)
        {
            mappedX = ResolutionLayout_Clamp(mappedX, 0.0, logicalWidth);
            mappedY = ResolutionLayout_Clamp(mappedY, 0.0, logicalHeight);
        }
    }

    if (logicalX)
        *logicalX = mappedX;
    if (logicalY)
        *logicalY = mappedY;
}

long long ResolutionLayout_ScaledMouseRange(long long baseRange, int logicalExtent, int windowExtent)
{
    long long result;

    if (baseRange <= 0 || logicalExtent <= 0 || windowExtent <= 0)
        return baseRange > 0 ? baseRange : 1;

    result = (long long)((double)baseRange * ((double)logicalExtent / (double)windowExtent));
    return result > 0 ? result : 1;
}
