#pragma once

#include "RenderPass.h"
#include <Device/Device.h>
#include <Camera/Camera.h>
#include <Geometry/Geometry.h>
#include <ProgramRef/ShadowPassVS.h>
#include <ProgramRef/ShadowPassGS.h>
#include <ProgramRef/ShadowPassPS.h>
#include "SponzaSettings.h"

class ShadowPass : public IPass, public IModifySponzaSettings
{
public:
    struct Input
    {
        SceneModels& scene_list;
        const Camera& camera;
        glm::vec3& light_pos;
    };

    struct Output
    {
        std::shared_ptr<Resource> srv;
    } output;

    ShadowPass(RenderDevice& device, const Input& input);

    virtual void OnUpdate() override;
    virtual void OnRender(RenderCommandList& command_list)override;
    virtual void OnModifySponzaSettings(const SponzaSettings& settings) override;

private:
    void CreateSizeDependentResources();

    SponzaSettings m_settings;
    RenderDevice& m_device;
    Input m_input;
    ProgramHolder<ShadowPassVS, ShadowPassGS, ShadowPassPS> m_program;
    std::shared_ptr<Resource> m_sampler;
};

