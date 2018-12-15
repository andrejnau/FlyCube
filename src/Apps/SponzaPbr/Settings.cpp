#include "Settings.h"

Settings::Settings()
{
    msaa_count = 1;
    width = 1280;
    height = 720;
    use_tone_mapping = false;
    use_simple_hdr = true;
    use_occlusion = true;
    use_white_ligth = false;
    enable_diffuse_for_metal = false;
    use_IBL_ambient = false;
    skip_sponza_model = false;
    only_ambient = false;
    ssao_scale = 1;
    Exposure = 0.1f;
    White = 3.0f;
}
