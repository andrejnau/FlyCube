#pragma once

#include "GeometryPass.h"
#include "Settings.h"
#include <Context/DX11Context.h>
#include <Geometry/Geometry.h>
#include <ProgramRef/CubemapVS.h>
#include <ProgramRef/Equirectangular2CubemapPS.h>
#include <ProgramRef/IrradianceConvolutionPS.h>
#include <ProgramRef/PrefilterPS.h>
#include <ProgramRef/DownSampleCS.h>
#include <d3d11.h>
#include <wrl.h>

using namespace Microsoft::WRL;

class Equirectangular2Cubemap : public IPass, public IModifySettings
{
public:
    struct Input
    {
        Model& model;
        Resource::Ptr& hdr;
    };

    struct Output
    {
        Resource::Ptr environment;
    } output;

    Equirectangular2Cubemap(Context& context, const Input& input, int width, int height);

    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnResize(int width, int height) override;
    virtual void OnModifySettings(const Settings & settings) override;

private:
    void DrawEquirectangular2Cubemap();
    void CreateSizeDependentResources();

    Settings m_settings;
    Context& m_context;
    Input m_input;
    int m_width;
    int m_height;
    Resource::Ptr m_sampler;
    Resource::Ptr m_dsv;
    Program<CubemapVS, Equirectangular2CubemapPS> m_program_equirectangular2cubemap;
    Program<DownSampleCS> m_program_downsample;
    size_t m_texture_size = 512;
    size_t m_texture_mips = 0;
    bool is = false;
};
