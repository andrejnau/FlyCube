#pragma once

#include "GeometryPass.h"
#include "SponzaSettings.h"
#include <Device/Device.h>
#include <Camera/Camera.h>
#include <Geometry/Geometry.h>
#include <ProgramRef/SSAOPassPS.h>
#include <ProgramRef/SSAOPassVS.h>
#include <ProgramRef/SSAOBlurPassPS.h>

class SSAOPass : public IPass, public IModifySponzaSettings
{
public:
    struct Input
    {
        GeometryPass::Output& geometry_pass;
        Model& square;
        const Camera& camera;
    };

    struct Output
    {
        std::shared_ptr<Resource> ao;
    } output;

    SSAOPass(RenderDevice& device, RenderCommandList& command_list, const Input& input, int width, int height);

    virtual void OnUpdate() override;
    virtual void OnRender(RenderCommandList& command_list)override;
    virtual void OnResize(int width, int height) override;
    virtual void OnModifySponzaSettings(const SponzaSettings& settings) override;

private:
    void SetDefines(ProgramHolder<SSAOPassPS, SSAOPassVS>& program);
    void CreateSizeDependentResources();

    SponzaSettings m_settings;
    RenderDevice& m_device;
    Input m_input;
    int m_width;
    int m_height;
    std::shared_ptr<Resource> m_noise_texture;
    std::shared_ptr<Resource> m_depth_stencil_view;
    ProgramHolder<SSAOPassPS, SSAOPassVS> m_program;
    ProgramHolder<SSAOBlurPassPS, SSAOPassVS> m_program_blur;
    std::shared_ptr<Resource> m_ao;
    std::shared_ptr<Resource> m_ao_blur;
    std::shared_ptr<Resource> m_sampler;
};
