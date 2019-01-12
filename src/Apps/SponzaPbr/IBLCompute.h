#pragma once

#include <vector>
#include <memory>
#include <Scene/SceneBase.h>
#include <Context/DX11Context.h>
#include <Geometry/Geometry.h>
#include <ProgramRef/IBLComputeVS.h>
#include <ProgramRef/IBLComputeGS.h>
#include <ProgramRef/IBLComputePS.h>
#include <ProgramRef/DownSampleCS.h>
#include <ProgramRef/BackgroundPS.h>
#include <ProgramRef/BackgroundVS.h>
#include <d3d11.h>
#include <wrl.h>
#include "Settings.h"

using namespace Microsoft::WRL;

class IBLCompute : public IPass, public IModifySettings
{
public:
    struct Input
    {
        SceneModels& scene_list;
        Camera& camera;
        Model& model_cube;
        Resource::Ptr& environment;
    };

    struct Output
    {
    } output;

    IBLCompute(Context& context, const Input& input, int width, int height);

    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnResize(int width, int height) override;
    virtual void OnModifySettings(const Settings & settings) override;

private:
    Settings m_settings;
    Context& m_context;
    Input m_input;
    Program<IBLComputeVS, IBLComputeGS, IBLComputePS> m_program;
    Program<BackgroundVS, BackgroundPS> m_program_backgroud;
    Program<DownSampleCS> m_program_downsample;
    Resource::Ptr m_dsv;
    Resource::Ptr m_sampler;
    size_t m_size = 512;
    int m_width;
    int m_height;
};
