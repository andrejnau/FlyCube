#include "Settings.h"

Settings::Settings()
{
    msaa_count = 1;
    width = 1280;
    height = 720;
    s_near = 0.5f;
    s_far = 1024.0f;
    s_size = 3072;
    use_shadow = true;
    use_tone_mapping = false;
    use_occlusion = true;
    use_blinn = false;
    light_ambient = 0.2f;
    light_diffuse = 1.0f;
    light_specular = 1.0f;
    ssao_scale = 1;
    Exposure = 0.1f;
    White = 3.0f;
}
