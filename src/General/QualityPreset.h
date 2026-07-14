#ifndef OPENJKDF2_QUALITY_PRESET_H
#define OPENJKDF2_QUALITY_PRESET_H

typedef enum QualityPreset
{
    QUALITY_PRESET_CLASSIC = 0,
    QUALITY_PRESET_BALANCED = 1,
    QUALITY_PRESET_HIGH = 2,
    QUALITY_PRESET_ULTRA = 3,
    QUALITY_PRESET_CUSTOM = 4
} QualityPreset;

typedef struct QualityPresetSettings
{
    int textureFiltering;
    int anisotropy;
    double mipmapBias;
    int bloom;
    int ssao;
} QualityPresetSettings;

QualityPreset QualityPreset_Normalize(int value);
QualityPresetSettings QualityPreset_Get(int preset);
int QualityPreset_AnisotropyFromSlider(int position);
int QualityPreset_SliderFromAnisotropy(int anisotropy);
int QualityPreset_Matches(int preset, int textureFiltering, int anisotropy, double mipmapBias, int bloom, int ssao);

#endif
