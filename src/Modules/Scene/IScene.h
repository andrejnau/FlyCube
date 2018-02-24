#pragma once

#include <memory>

enum class ApiType
{
    kOpenGL,
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
    bool use_tone_mapping = true;
    bool use_occlusion = true;
    float light_ambient = 0.2;
    float light_diffuse = 1.0;
    float light_specular = 1.0;
    int ssao_scale = 1;
    float Exposure = 0.1;
    float White = 3;
};

class IModifySettings
{
public:
    virtual void OnModifySettings(const Settings& settings) = 0;
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
    virtual void OnKey(int key, int action) = 0;
    virtual void OnMouse(bool first, double xpos, double ypos) = 0;
    virtual void OnMouseButton(int button, int action) = 0;
    virtual void OnScroll(double xoffset, double yoffset) = 0;
    virtual void OnInputChar(unsigned int ch) = 0;
};

class IScene
    : public IPass
    , public IInput
    , public IModifySettings
{
public:
    using Ptr = std::unique_ptr<IScene>;
};