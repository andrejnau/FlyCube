#pragma once

#include "GeometryPass.h"
#include "Settings.h"
#include <Context/DX11Context.h>
#include <Geometry/Geometry.h>
#include <ProgramRef/CubemapVS.h>
#include <ProgramRef/Equirectangular2CubemapPS.h>
#include <ProgramRef/IrradianceConvolutionPS.h>
#include <ProgramRef/PrefilterPS.h>
#include <ProgramRef/BRDFPS.h>
#include <ProgramRef/BRDFVS.h>
#include <ProgramRef/DownSampleCS.h>
#include <d3d11.h>
#include <wrl.h>

using namespace Microsoft::WRL;

class IrradianceConversion : public IPass, public IModifySettings
{
public:
    struct Input
    {
        Model& model;
        Model& square_model;
        Resource::Ptr& hdr;
    };

    struct Output
    {
        Resource::Ptr environment;
        Resource::Ptr irradince;
        Resource::Ptr prefilter;
        Resource::Ptr brdf;
    } output;

    IrradianceConversion(Context& context, const Input& input, int width, int height);

    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnResize(int width, int height) override;
    virtual void OnModifySettings(const Settings & settings) override;

private:
    void DrawEquirectangular2Cubemap();
    void DrawIrradianceConvolution();
    void DrawPrefilter();
    void DrawBRDF();
    void CreateSizeDependentResources();

    Settings m_settings;
    Context& m_context;
    Input m_input;
    int m_width;
    int m_height;
    Resource::Ptr m_sampler;
    Resource::Ptr m_depth_stencil_view;
    Program<CubemapVS, Equirectangular2CubemapPS> m_program_equirectangular2cubemap;
    Program<CubemapVS, IrradianceConvolutionPS> m_program_irradiance_convolution;
    Program<CubemapVS, PrefilterPS> m_program_prefilter;
    Program<BRDFVS, BRDFPS> m_program_brdf;
    Program<DownSampleCS> m_program_downsample;
    size_t m_texture_size = 512;
    size_t m_texture_mips = 0;
    size_t m_irradince_texture_size = 32;
    size_t m_prefilter_texture_size = 128;
    size_t m_brdf_size = 512;
};
