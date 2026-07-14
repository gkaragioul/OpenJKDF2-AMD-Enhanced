#include "General/MouseSmoothing.h"

#include <assert.h>

int main(void)
{
    assert(MouseSmoothing_Filter(10, 10) == 10);
    assert(MouseSmoothing_Filter(10, 0) == 7);
    assert(MouseSmoothing_Filter(0, 10) == 4);
    assert(MouseSmoothing_Filter(-10, 0) == -7);
    assert(MouseSmoothing_Filter(1, 0) == 1);
    assert(MouseSmoothing_Filter(-1, 0) == -1);
    return 0;
}
