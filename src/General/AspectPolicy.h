#ifndef OPENJKDF2_ASPECT_POLICY_H
#define OPENJKDF2_ASPECT_POLICY_H

#include "General/ResolutionLayout.h"

ResolutionLayoutRect AspectPolicy_Destination(
    int outputWidth,
    int outputHeight,
    int contentWidth,
    int contentHeight,
    int preserveAspect);

#endif
