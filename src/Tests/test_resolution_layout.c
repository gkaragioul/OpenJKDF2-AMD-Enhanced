#include "General/ResolutionLayout.h"

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
    ResolutionLayoutRect rect;
    double x;
    double y;

    assert_rect(ResolutionLayout_FitAspect(1920, 1080, 4.0 / 3.0), 240.0, 0.0, 1440.0, 1080.0);
    assert_rect(ResolutionLayout_FitAspect(2560, 1440, 4.0 / 3.0), 320.0, 0.0, 1920.0, 1440.0);
    assert_rect(ResolutionLayout_FitAspect(3840, 2160, 4.0 / 3.0), 480.0, 0.0, 2880.0, 2160.0);
    assert_rect(ResolutionLayout_FitAspect(1080, 1920, 4.0 / 3.0), 0.0, 555.0, 1080.0, 810.0);
    assert_rect(ResolutionLayout_FitAspect(0, 1440, 4.0 / 3.0), 0.0, 0.0, 0.0, 0.0);

    rect = ResolutionLayout_FitAspect(2560, 1440, 4.0 / 3.0);
    ResolutionLayout_MapPoint(&rect, 1280.0, 720.0, 640.0, 480.0, 1, &x, &y);
    assert_close(x, 320.0);
    assert_close(y, 240.0);

    ResolutionLayout_MapPoint(&rect, 0.0, -100.0, 640.0, 480.0, 1, &x, &y);
    assert_close(x, 0.0);
    assert_close(y, 0.0);
    ResolutionLayout_MapPoint(&rect, 3000.0, 2000.0, 640.0, 480.0, 1, &x, &y);
    assert_close(x, 640.0);
    assert_close(y, 480.0);

    assert(ResolutionLayout_ScaledMouseRange(250, 640, 2560) == 62);
    assert(ResolutionLayout_ScaledMouseRange(200, 480, 1440) == 66);
    assert(ResolutionLayout_ScaledMouseRange(200, 480, 0) == 200);
    assert(ResolutionLayout_ScaledMouseRange(1, 1, 100000) == 1);

    return 0;
}
