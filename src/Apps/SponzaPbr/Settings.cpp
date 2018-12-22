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
    use_ssao = true;
    use_white_ligth = false;
    use_IBL_diffuse = true;
    use_IBL_specular = true;
    skip_sponza_model = false;
    only_ambient = false;
    light_in_camera = false;
    show_only_normal = false;
    use_flip_normal_y = false;
    use_spec_ao_by_ndotv_roughness = true;
    irradiance_conversion_every_frame = false;
    ambient_power = 1.0;
    light_power = 1.0;
    ssao_scale = 1;
    Exposure = 0.1f;
    White = 3.0f;
}
