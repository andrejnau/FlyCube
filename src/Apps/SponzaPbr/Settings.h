#pragma once

#include <cstdint>

class Settings
{
public:
    Settings();

    uint32_t msaa_count;
    int width;
    int height;
    bool gamma_correction;
    bool use_reinhard_tone_operator;
    bool use_tone_mapping;
    bool use_white_balance;
    bool use_filmic_hdr;
    bool use_avg_lum;
    bool use_ao;
    bool use_ssao;
    bool use_white_ligth;
    bool only_ambient;
    bool use_IBL_diffuse;
    bool use_IBL_specular;
    bool skip_sponza_model;
    bool light_in_camera;
    bool additional_lights;
    bool show_only_albedo;
    bool show_only_normal;
    bool show_only_roughness;
    bool show_only_metalness;
    bool show_only_ao;
    bool use_f0_with_roughness;
    bool use_flip_normal_y;
    bool use_spec_ao_by_ndotv_roughness;
    bool irradiance_conversion_every_frame;
    float ambient_power;
    float light_power;
    float exposure;
    float white;
    float s_near;
    float s_far;
    float s_size;
    bool use_shadow;
    bool normal_mapping;
    bool shadow_discard;
    bool dynamic_sun_position;
};

class IModifySettings
{
public:
    virtual void OnModifySettings(const Settings& settings) {}
};
