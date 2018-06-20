#pragma once

#include <memory>

enum class ApiType
{
    kOpenGL,
    kVulkan,
    kDX11,
    kDX12
};

class Settings
{
public:
    uint32_t msaa_count = 1;
    int width = 1280;
    int height = 720;
    float s_near = 0.5f;
    float s_far = 1024.0f;
    int s_size = 3072;
    bool use_shadow = true;
    bool use_tone_mapping = true;
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

class IPass
{
public:
    virtual ~IPass() = default;
    virtual void OnUpdate() = 0;
    virtual void OnRender() = 0;
    virtual void OnResize(int width, int height) = 0;
};

class IInput
{
public:
    virtual ~IInput() = default;
    virtual void OnKey(int key, int action) {}
    virtual void OnMouse(bool first, double xpos, double ypos) {}
    virtual void OnMouseButton(int button, int action) {}
    virtual void OnScroll(double xoffset, double yoffset) {}
    virtual void OnInputChar(unsigned int ch) {}
};

class IScene
    : public IPass
    , public IInput
    , public IModifySettings
{
public:
    using Ptr = std::unique_ptr<IScene>;
};