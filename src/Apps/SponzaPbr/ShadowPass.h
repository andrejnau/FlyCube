#pragma once

#include <Scene/SceneBase.h>
#include <Context/DX11Context.h>
#include <Geometry/Geometry.h>
#include <ProgramRef/ShadowPassVS.h>
#include <ProgramRef/ShadowPassGS.h>
#include <ProgramRef/ShadowPassPS.h>
#include <d3d11.h>
#include <wrl.h>
#include "Settings.h"

using namespace Microsoft::WRL;

class ShadowPass : public IPass, public IModifySettings
{
public:
    struct Input
    {
        SceneModels& scene_list;
        Camera& camera;
        glm::vec3& light_pos;
    };

    struct Output
    {
        Resource::Ptr srv;
    } output;

    ShadowPass(Context& context, const Input& input, int width, int height);

    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnResize(int width, int height) override;
    virtual void OnModifySettings(const Settings & settings) override;

private:
    void CreateSizeDependentResources();

    Settings m_settings;
    Context& m_context;
    Input m_input;
    Program<ShadowPassVS, ShadowPassGS, ShadowPassPS> m_program;
    Resource::Ptr m_buffer;
    Resource::Ptr m_sampler;
};

