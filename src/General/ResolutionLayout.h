#ifndef OPENJKDF2_RESOLUTION_LAYOUT_H
#define OPENJKDF2_RESOLUTION_LAYOUT_H

typedef struct ResolutionLayoutRect
{
    double x;
    double y;
    double width;
    double height;
} ResolutionLayoutRect;

ResolutionLayoutRect ResolutionLayout_FitAspect(int outputWidth, int outputHeight, double contentAspect);
void ResolutionLayout_MapPoint(
    const ResolutionLayoutRect* viewport,
    double x,
    double y,
    double logicalWidth,
    double logicalHeight,
    int clampToBounds,
    double* logicalX,
    double* logicalY);
long long ResolutionLayout_ScaledMouseRange(long long baseRange, int logicalExtent, int windowExtent);

#endif
