#include "General/AspectPolicy.h"

#include <assert.h>
#include <math.h>

static void assert_close(double actual, double expected)
{
    assert(fabs(actual - expected) < 0.000001);
}

static void assert_rect(ResolutionLayoutRect rect, double x, double y, double width, double height)
{
    assert_close(rect.x, x);
    assert_close(rect.y, y);
    assert_close(rect.width, width);
    assert_close(rect.height, height);
}

int main(void)
{
    assert_rect(AspectPolicy_Destination(2560, 1440, 640, 480, 0), 0, 0, 2560, 1440);
    assert_rect(AspectPolicy_Destination(2560, 1440, 640, 480, 1), 320, 0, 1920, 1440);
    assert_rect(AspectPolicy_Destination(3440, 1440, 640, 480, 1), 760, 0, 1920, 1440);
    assert_rect(AspectPolicy_Destination(1080, 1920, 640, 480, 1), 0, 555, 1080, 810);
    assert_rect(AspectPolicy_Destination(2560, 1440, 640, 360, 1), 0, 0, 2560, 1440);
    assert_rect(AspectPolicy_Destination(2560, 1440, 640, 300, 1), 0, 120, 2560, 1200);
    assert_rect(AspectPolicy_Destination(3840, 2160, 640, 480, 1), 480, 0, 2880, 2160);
    assert_rect(AspectPolicy_Destination(0, 1440, 640, 480, 1), 0, 0, 0, 0);
    assert_rect(AspectPolicy_Destination(2560, 1440, 0, 480, 1), 0, 0, 0, 0);
    return 0;
}
