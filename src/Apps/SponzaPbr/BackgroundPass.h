#pragma once

#include "GeometryPass.h"
#include "SponzaSettings.h"
#include <Device/Device.h>
#include <Geometry/Geometry.h>
#include <ProgramRef/BackgroundPS.h>
#include <ProgramRef/BackgroundVS.h>

class BackgroundPass : public IPass, public IModifySponzaSettings
{
public:
    struct Input
    {
        Model& model;
        const Camera& camera;
        std::shared_ptr<Resource>& environment;
        std::shared_ptr<Resource>& rtv;
        std::shared_ptr<Resource>& dsv;
    };

    struct Output
    {
        std::shared_ptr<Resource> environment;
        std::shared_ptr<Resource> irradince;
    } output;

    BackgroundPass(Device& device, const Input& input, int width, int height);

    virtual void OnUpdate() override;
    virtual void OnRender(RenderCommandList& command_list)override;
    virtual void OnResize(int width, int height) override;
    virtual void OnModifySponzaSettings(const SponzaSettings& settings) override;

private:
    SponzaSettings m_settings;
    Device& m_device;
    Input m_input;
    int m_width;
    int m_height;
    std::shared_ptr<Resource> m_sampler;
    ProgramHolder<BackgroundVS, BackgroundPS> m_program;
};
