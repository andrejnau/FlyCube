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
    bool use_simple_hdr2;
    bool use_ao;
    bool use_ssao;
    bool use_white_ligth;
    bool only_ambient;
    bool use_IBL_diffuse;
    bool use_IBL_specular;
    bool skip_sponza_model;
    bool irradiance_conversion_every_frame;
    float light_power;
    int ssao_scale;
    float Exposure;
    float White;
};

class IModifySettings
{
public:
    virtual void OnModifySettings(const Settings& settings) {}
};
