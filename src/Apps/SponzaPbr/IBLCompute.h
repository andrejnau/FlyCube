#pragma once

#include <vector>
#include <memory>
#include "RenderPass.h"
#include <Device/Device.h>
#include <Camera/Camera.h>
#include <Geometry/Geometry.h>
#include <ProgramRef/IBLComputeVS.h>
#include <ProgramRef/IBLComputeGS.h>
#include <ProgramRef/IBLComputePS.h>
#include <ProgramRef/IBLComputePrePassPS.h>
#include <ProgramRef/DownSampleCS.h>
#include <ProgramRef/BackgroundPS.h>
#include <ProgramRef/BackgroundVS.h>
#include "SponzaSettings.h"
#include "ShadowPass.h"

class IBLCompute : public IPass, public IModifySponzaSettings
{
public:
    struct Input
    {
        ShadowPass::Output& shadow_pass;
        SceneModels& scene_list;
        const Camera& camera;
        glm::vec3& light_pos;
        Model& model_cube;
        std::shared_ptr<Resource>& environment;
    };

    struct Output
    {
    } output;

    IBLCompute(Device& device, const Input& input);

    virtual void OnUpdate() override;
    virtual void OnRender(CommandListBox& command_list) override;
    virtual void OnModifySponzaSettings(const SponzaSettings& settings) override;

private:
    void DrawPrePass(CommandListBox& command_list, Model& ibl_model);
    void Draw(CommandListBox& command_list, Model& ibl_model);
    void DrawBackgroud(CommandListBox& command_list, Model& ibl_model);
    void DrawDownSample(CommandListBox& command_list, Model& ibl_model, size_t texture_mips);
    SponzaSettings m_settings;
    Device& m_device;
    Input m_input;
    ProgramHolder<IBLComputeVS, IBLComputeGS, IBLComputePS> m_program;
    ProgramHolder<IBLComputeVS, IBLComputeGS, IBLComputePrePassPS> m_program_pre_pass;
    ProgramHolder<BackgroundVS, BackgroundPS> m_program_backgroud;
    ProgramHolder<DownSampleCS> m_program_downsample;
    std::shared_ptr<Resource> m_dsv;
    std::shared_ptr<Resource> m_sampler;
    std::shared_ptr<Resource> m_compare_sampler;
    size_t m_size = 512;
    bool m_use_pre_pass = true;
};
