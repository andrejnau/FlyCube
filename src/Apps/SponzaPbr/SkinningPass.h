#pragma once

#include "RenderPass.h"
#include "SponzaSettings.h"
#include <Device/Device.h>
#include <Geometry/Geometry.h>
#include <ProgramRef/SkinningCS.h>

class SkinningPass : public IPass, public IModifySponzaSettings
{
public:
    struct Input
    {
        SceneModels& scene_list;
    };

    struct Output
    {
    } output;

    SkinningPass(Device& device, const Input& input, int width, int height);

    virtual void OnUpdate() override;
    virtual void OnRender(CommandListBox& command_list)override;
    virtual void OnModifySponzaSettings(const SponzaSettings& settings) override;

private:
    SponzaSettings m_settings;
    Device& m_device;
    Input m_input;
    ProgramHolder<SkinningCS> m_program;
};
