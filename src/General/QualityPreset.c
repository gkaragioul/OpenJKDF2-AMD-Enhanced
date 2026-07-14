#include "General/QualityPreset.h"

#include <math.h>

QualityPreset QualityPreset_Normalize(int value)
{
    if (value >= QUALITY_PRESET_CLASSIC && value <= QUALITY_PRESET_ULTRA)
        return (QualityPreset)value;
    return QUALITY_PRESET_CUSTOM;
}

QualityPresetSettings QualityPreset_Get(int preset)
{
    QualityPresetSettings settings = {1, 4, 0.75, 0, 0, 1.0, 1, 1};

    switch (QualityPreset_Normalize(preset))
    {
        case QUALITY_PRESET_CLASSIC:
            settings.textureFiltering = 0;
            settings.anisotropy = 1;
            settings.mipmapBias = 1.0;
            settings.texturePrecache = 0;
            settings.assetEnhancements = 0;
            break;
        case QUALITY_PRESET_HIGH:
            settings.anisotropy = 8;
            settings.mipmapBias = 0.65;
            settings.bloom = 1;
            settings.ssaaMultiple = 1.25;
            break;
        case QUALITY_PRESET_ULTRA:
            settings.anisotropy = 16;
            settings.mipmapBias = 0.5;
            settings.bloom = 1;
            settings.ssao = 1;
            settings.ssaaMultiple = 1.5;
            break;
        case QUALITY_PRESET_CUSTOM:
        case QUALITY_PRESET_BALANCED:
        default:
            break;
    }
    return settings;
}

int QualityPreset_AnisotropyFromSlider(int position)
{
    if (position <= 0)
        return 1;
    if (position >= 4)
        return 16;
    return 1 << position;
}

int QualityPreset_SliderFromAnisotropy(int anisotropy)
{
    if (anisotropy <= 1)
        return 0;
    if (anisotropy <= 2)
        return 1;
    if (anisotropy <= 4)
        return 2;
    if (anisotropy <= 8)
        return 3;
    return 4;
}

double QualityPreset_ClampSsaa(double value)
{
    if (!isfinite(value) || value < 1.0)
        return 1.0;
    if (value > 2.0)
        return 2.0;
    return value;
}

int QualityPreset_Matches(int preset, int textureFiltering, int anisotropy, double mipmapBias,
                          int bloom, int ssao, double ssaaMultiple, int texturePrecache,
                          int assetEnhancements)
{
    QualityPresetSettings settings;
    QualityPreset normalized = QualityPreset_Normalize(preset);

    if (normalized == QUALITY_PRESET_CUSTOM)
        return 0;

    settings = QualityPreset_Get(normalized);
    return settings.textureFiltering == !!textureFiltering
        && settings.anisotropy == anisotropy
        && fabs(settings.mipmapBias - mipmapBias) < 0.001
        && settings.bloom == !!bloom
        && settings.ssao == !!ssao
        && fabs(settings.ssaaMultiple - ssaaMultiple) < 0.001
        && settings.texturePrecache == !!texturePrecache
        && settings.assetEnhancements == !!assetEnhancements;
}
