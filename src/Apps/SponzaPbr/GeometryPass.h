#pragma once

#include "Settings.h"
#include <Scene/SceneBase.h>
#include <Context/DX11Context.h>
#include <Geometry/Geometry.h>
#include <ProgramRef/GeometryPassPS.h>
#include <ProgramRef/GeometryPassVS.h>
#include <d3d11.h>
#include <wrl.h>

using namespace Microsoft::WRL;

class GeometryPass : public IPass, public IModifySettings
{
public:
    struct Input
    {
        SceneModels& scene_list;
        Camera& camera;
    };

    struct Output
    {
        Resource::Ptr position;
        Resource::Ptr normal;
        Resource::Ptr albedo;
        Resource::Ptr roughness;
        Resource::Ptr metalness;
        Resource::Ptr dsv;
    } output;

    GeometryPass(Context& context, const Input& input, int width, int height);

    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnResize(int width, int height) override;
    virtual void OnModifySettings(const Settings& settings) override;

private:
    Context& m_context;
    Input m_input;
    int m_width;
    int m_height;
    Program<GeometryPassPS, GeometryPassVS> m_program;

    void CreateSizeDependentResources();

    Resource::Ptr m_sampler;
    Settings m_settings;
};
