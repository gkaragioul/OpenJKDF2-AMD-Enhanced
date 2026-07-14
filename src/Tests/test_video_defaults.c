#include "General/VideoDefaults.h"

#include <stdio.h>

static int failures;
#define CHECK(value) do { if (!(value)) { fprintf(stderr, "line %d: %s\n", __LINE__, #value); ++failures; } } while (0)

int main(void)
{
    VideoDefaults recommended = video_defaults_recommended();
    VideoDefaults safe = video_defaults_safe();

    CHECK(recommended.display_mode == DISPLAY_MODE_BORDERLESS);
    CHECK(recommended.fps_limit == FRAME_RATE_DESKTOP_REFRESH);
    CHECK(recommended.vsync == PRESENTATION_VSYNC_ON);
    CHECK(recommended.quality_preset == QUALITY_PRESET_BALANCED);
    CHECK(recommended.texture_filtering == 1);
    CHECK(recommended.anisotropy == 4);
    CHECK(recommended.bloom == 0);
    CHECK(recommended.ssao == 0);
    CHECK(recommended.ssaa_multiple == 1.0);
    CHECK(recommended.gamma == 1.0);
    CHECK(recommended.fov == 90);
    CHECK(recommended.hud_scale == 2.0);

    CHECK(safe.display_mode == DISPLAY_MODE_WINDOWED);
    CHECK(safe.hidpi == 0);
    CHECK(safe.fps_limit == 60);
    CHECK(safe.vsync == PRESENTATION_VSYNC_ON);
    CHECK(safe.quality_preset == QUALITY_PRESET_CLASSIC);
    CHECK(safe.texture_filtering == 0);
    CHECK(safe.anisotropy == 1);
    CHECK(safe.bloom == 0);
    CHECK(safe.ssao == 0);
    CHECK(safe.ssaa_multiple == 1.0);

    return failures ? 1 : 0;
}
