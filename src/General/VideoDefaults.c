#include "General/VideoDefaults.h"

VideoDefaults video_defaults_recommended(void)
{
    VideoDefaults defaults = {
        DISPLAY_MODE_BORDERLESS,
        0,
        90,
        1,
        0,
        FRAME_RATE_DESKTOP_REFRESH,
        PRESENTATION_VSYNC_ON,
        QUALITY_PRESET_BALANCED,
        1,
        4,
        0.75,
        0,
        0,
        1.0,
        1.0,
        2.0,
        1,
        1
    };
    return defaults;
}

VideoDefaults video_defaults_safe(void)
{
    VideoDefaults defaults = video_defaults_recommended();
    defaults.display_mode = DISPLAY_MODE_WINDOWED;
    defaults.fps_limit = 60;
    defaults.quality_preset = QUALITY_PRESET_CLASSIC;
    defaults.texture_filtering = 0;
    defaults.anisotropy = 1;
    defaults.mipmap_bias = 1.0;
    defaults.texture_precache = 0;
    defaults.asset_enhancements = 0;
    return defaults;
}
