#pragma once

class Settings
{
public:
    uint32_t msaa_count = 1;
    int width = 1280;
    int height = 720;
    float s_near = 0.5f;
    float s_far = 1024.0f;
    int s_size = 3072;
    bool use_shadow = false;
    bool use_tone_mapping = false;
    bool use_occlusion = true;
    bool use_blinn = false;
    float light_ambient = 0.2f;
    float light_diffuse = 1.0f;
    float light_specular = 1.0f;
    int ssao_scale = 1;
    float Exposure = 0.1f;
    float White = 3.0f;
};

class IModifySettings
{
public:
    virtual void OnModifySettings(const Settings& settings) {}
};
