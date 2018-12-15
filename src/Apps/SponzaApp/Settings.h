#pragma once

#include <stdint.h>

class Settings
{
public:
    Settings();

    uint32_t msaa_count;
    int width;
    int height;
    float s_near;
    float s_far;
    int s_size;
    bool use_shadow;
    bool use_tone_mapping;
    bool use_occlusion;
    bool use_blinn;
    float light_ambient;
    float light_diffuse;
    float light_specular;
    int ssao_scale;
    float Exposure;
    float White;
};

class IModifySettings
{
public:
    virtual void OnModifySettings(const Settings& settings) {}
};
  