#pragma once

#include <Scene/SceneBase.h>
#include <Context/Context.h>
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
        ComPtr<ID3D11Resource> position;
        ComPtr<ID3D11Resource> normal;
        ComPtr<ID3D11Resource> ambient;
        ComPtr<ID3D11Resource> diffuse;
        ComPtr<ID3D11Resource> specular;
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

    void InitGBuffers();

    ComPtr<ID3D11Resource> m_depth_stencil;
    Settings m_settings;
};

