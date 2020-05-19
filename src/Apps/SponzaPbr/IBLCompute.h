#pragma once

#include <vector>
#include <memory>
#include <Scene/SceneBase.h>
#include <Context/Context.h>
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
        Camera& camera;
        glm::vec3& light_pos;
        Model& model_cube;
        std::shared_ptr<Resource>& environment;
    };

    struct Output
    {
    } output;

    IBLCompute(Context& context, const Input& input, int width, int height);

    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnResize(int width, int height) override;
    virtual void OnModifySponzaSettings(const SponzaSettings& settings) override;

private:
    void DrawPrePass(Model& ibl_model);
    void Draw(Model& ibl_model);
    void DrawBackgroud(Model& ibl_model);
    void DrawDownSample(Model& ibl_model, size_t texture_mips);
    SponzaSettings m_settings;
    Context& m_context;
    Input m_input;
    ProgramHolder<IBLComputeVS, IBLComputeGS, IBLComputePS> m_program;
    ProgramHolder<IBLComputeVS, IBLComputeGS, IBLComputePrePassPS> m_program_pre_pass;
    ProgramHolder<BackgroundVS, BackgroundPS> m_program_backgroud;
    ProgramHolder<DownSampleCS> m_program_downsample;
    std::shared_ptr<Resource> m_dsv;
    std::shared_ptr<Resource> m_sampler;
    std::shared_ptr<Resource> m_compare_sampler;
    size_t m_size = 512;
    int m_width;
    int m_height;
    bool m_use_pre_pass = true;
};
