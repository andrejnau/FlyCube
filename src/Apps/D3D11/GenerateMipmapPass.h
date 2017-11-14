#pragma once

#include "D3D11/GeometryPass.h"
#include "D3D11/ShadowPass.h"

#include <Scene/SceneBase.h>
#include <Context/Context.h>
#include <Geometry/DX11Geometry.h>
#include <ProgramRef/GenerateMipmapCS.h>
#include <d3d11.h>
#include <wrl.h>

using namespace Microsoft::WRL;

class GenerateMipmapPass : public IPass
{
public:
    struct Input
    {
        ShadowPass::Output& shadow_pass;
    };

    struct Output
    {
    } output;

    GenerateMipmapPass(Context& context, const Input& input, int width, int height);

    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnResize(int width, int height) override;

private:
    Context& m_context;
    Input m_input;
    int m_width;
    int m_height;
    ComPtr<ID3D11SamplerState> m_shadow_sampler;
    Program<GenerateMipmapCS> m_program;
};
