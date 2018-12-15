#pragma once

#include <cstdint>

class Settings
{
public:
    Settings();

    uint32_t msaa_count;
    int width;
    int height;
    bool use_tone_mapping;
    bool use_simple_hdr;
    bool use_occlusion;
    bool use_white_ligth;
    bool enable_diffuse_for_metal;
    int ssao_scale;
    float Exposure;
    float White;
};

class IModifySettings
{
public:
    virtual void OnModifySettings(const Settings& settings) {}
};
