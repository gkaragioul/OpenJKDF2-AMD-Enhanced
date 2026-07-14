#include "General/FixedStep.h"

#include <assert.h>

static void assert_one_second_is_invariant(int renderRate)
{
    const double step = 1.0 / 150.0;
    double accumulator = 0.0;
    uint32_t totalSteps = 0;
    int frame;

    for (frame = 0; frame < renderRate; ++frame)
    {
        totalSteps += FixedStep_Consume(1.0 / (double)renderRate, step, 75, &accumulator);
        assert(accumulator >= 0.0);
        assert(accumulator < step);
    }

    assert(totalSteps == 150);
    assert(accumulator < 0.000000001);
}

int main(void)
{
    double accumulator = 0.0;

    assert_one_second_is_invariant(60);
    assert_one_second_is_invariant(120);
    assert_one_second_is_invariant(144);
    assert_one_second_is_invariant(165);
    assert_one_second_is_invariant(240);

    assert(FixedStep_Consume(0.5, 1.0 / 150.0, 10, &accumulator) == 10);
    assert(accumulator >= 0.0 && accumulator < 1.0 / 150.0);
    assert(FixedStep_Consume(-1.0, 1.0 / 150.0, 10, &accumulator) == 0);
    assert(FixedStep_Consume(1.0, 0.0, 10, &accumulator) == 0);
    assert(FixedStep_Consume(1.0, 1.0 / 150.0, 0, &accumulator) == 0);
    assert(FixedStep_Consume(1.0, 1.0 / 150.0, 10, 0) == 0);
    return 0;
}
