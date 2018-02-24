#pragma once

#include <Scene/SceneBase.h>
#include <Context/DX11Context.h>
#include <Geometry/DX11Geometry.h>
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
        SceneList<DX11Mesh>& scene_list;
        Camera& camera;
    };

    struct Output
    {
        ComPtr<IUnknown> position;
        ComPtr<IUnknown> normal;
        ComPtr<IUnknown> ambient;
        ComPtr<IUnknown> diffuse;
        ComPtr<IUnknown> specular;
    } output;

    GeometryPass(DX11Context& context, const Input& input, int width, int height);

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

    void InitGBuffers();

    ComPtr<IUnknown> m_depth_stencil;
    Settings m_settings;
    ComPtr<IUnknown> m_g_sampler;
};
