#pragma once

#include "D3D11/GeometryPass.h"
#include "D3D11/ShadowPass.h"
#include "D3D11/SSAOPass.h"

#include <Scene/SceneBase.h>
#include <Context/DX11Context.h>
#include <Geometry/Geometry.h>
#include <ProgramRef/LightPassPS.h>
#include <ProgramRef/LightPassVS.h>
#include <d3d11.h>
#include <wrl.h>

using namespace Microsoft::WRL;

class LightPass : public IPass, public IModifySettings
{
public:
    struct Input
    {
        GeometryPass::Output& geometry_pass;
        ShadowPass::Output& shadow_pass;
        SSAOPass::Output& ssao_pass;
        Model& model;
        Camera& camera;
        glm::vec3& light_pos;
    };

    struct Output
    {
        Resource::Ptr rtv;
    } output;

    void SetDefines(Program<LightPassPS, LightPassVS>& program);

    LightPass(Context& context, const Input& input, int width, int height);

    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnResize(int width, int height) override;
    virtual void OnModifySettings(const Settings & settings) override;

private:
    Settings m_settings;
    Context& m_context;
    Input m_input;
    int m_width;
    int m_height;
    Program<LightPassPS, LightPassVS> m_program;
    Resource::Ptr m_depth_stencil_view;
};
