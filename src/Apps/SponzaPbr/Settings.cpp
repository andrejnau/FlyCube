#include "Settings.h"
#include <cmath>

Settings::Settings()
{
    msaa_count = 1;
    width = 1280;
    height = 720;
    use_tone_mapping = true;
    use_simple_hdr = false;
    use_simple_hdr2 = false;
    use_ao = true;
    use_ssao = true;
    use_white_ligth = false;
    use_IBL_diffuse = false;
    use_IBL_specular = false;
    skip_sponza_model = false;
    only_ambient = false;
    light_in_camera = false;
    show_only_normal = false;
    use_flip_normal_y = false;
    use_spec_ao_by_ndotv_roughness = true;
    irradiance_conversion_every_frame = false;
    ambient_power = 0.2;
    light_power = acos(-1.0);
    ssao_scale = 1;
    Exposure = 0.1;
    White = 3.0;
    s_near = 0.5;
    s_far = 1024.0;
    s_size = 3072;
    use_shadow = true;
}
