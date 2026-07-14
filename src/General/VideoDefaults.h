#ifndef OPENJKDF2_VIDEO_DEFAULTS_H
#define OPENJKDF2_VIDEO_DEFAULTS_H

#include "General/DisplayMode.h"
#include "General/FrameRate.h"
#include "General/PresentationMode.h"
#include "General/QualityPreset.h"

typedef struct VideoDefaults
{
    DisplayMode display_mode;
    int hidpi;
    int fov;
    int fov_vertical;
    int original_aspect;
    int fps_limit;
    PresentationVsyncMode vsync;
    QualityPreset quality_preset;
    int texture_filtering;
    int anisotropy;
    double mipmap_bias;
    int bloom;
    int ssao;
    double ssaa_multiple;
    double gamma;
    double hud_scale;
    int texture_precache;
    int asset_enhancements;
} VideoDefaults;

VideoDefaults video_defaults_recommended(void);
VideoDefaults video_defaults_safe(void);

#endif
