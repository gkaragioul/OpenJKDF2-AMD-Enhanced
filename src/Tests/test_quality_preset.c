#include "General/QualityPreset.h"

#include <assert.h>

int main(void)
{
    QualityPresetSettings settings;

    settings = QualityPreset_Get(QUALITY_PRESET_CLASSIC);
    assert(!settings.textureFiltering);
    assert(settings.anisotropy == 1);
    assert(settings.mipmapBias == 1.0);

    settings = QualityPreset_Get(QUALITY_PRESET_HIGH);
    assert(settings.textureFiltering);
    assert(settings.anisotropy == 8);
    assert(settings.mipmapBias < 1.0);

    settings = QualityPreset_Get(QUALITY_PRESET_ULTRA);
    assert(settings.anisotropy == 16);
    assert(settings.bloom && settings.ssao);
    assert(QualityPreset_Normalize(99) == QUALITY_PRESET_CUSTOM);
    assert(QualityPreset_AnisotropyFromSlider(0) == 1);
    assert(QualityPreset_AnisotropyFromSlider(4) == 16);
    assert(QualityPreset_SliderFromAnisotropy(12) == 4);
    assert(QualityPreset_Matches(QUALITY_PRESET_HIGH, 1, 8, 0.65, 1, 0));
    assert(!QualityPreset_Matches(QUALITY_PRESET_HIGH, 0, 8, 0.65, 1, 0));
    assert(!QualityPreset_Matches(QUALITY_PRESET_HIGH, 1, 4, 0.65, 1, 0));
    assert(!QualityPreset_Matches(QUALITY_PRESET_HIGH, 1, 8, 0.75, 1, 0));
    assert(!QualityPreset_Matches(QUALITY_PRESET_CUSTOM, 1, 4, 0.75, 0, 0));
    return 0;
}
