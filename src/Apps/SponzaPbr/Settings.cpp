#include "Settings.h"

Settings::Settings()
{
    msaa_count = 1;
    width = 1280;
    height = 720;
    use_tone_mapping = false;
    use_simple_hdr = true;
    use_simple_hdr2 = false;
    use_ao = true;
    use_ssao = false;
    use_white_ligth = false;
    enable_diffuse_for_metal = false;
    use_IBL_diffuse = true;
    use_IBL_specular = true;
    swap_y_for_brdf = true;
    skip_sponza_model = false;
    only_ambient = false;
    irradiance_conversion_every_frame = false;
    light_power = 1.0;
    ssao_scale = 1;
    Exposure = 0.1f;
    White = 3.0f;
}
