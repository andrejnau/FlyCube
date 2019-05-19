#pragma once

#include "GeometryPass.h"
#include "Settings.h"
#include <Context/Context.h>
#include <Geometry/Geometry.h>
#include <ProgramRef/CubemapVS.h>
#include <ProgramRef/IrradianceConvolutionPS.h>
#include <ProgramRef/PrefilterPS.h>
#include <d3d11.h>
#include <wrl.h>

using namespace Microsoft::WRL;

class IrradianceConversion : public IPass, public IModifySettings
{
public:
    struct Target
    {
        Resource::Ptr& res;
        Resource::Ptr& dsv;
        size_t layer;
        size_t size;
    };

    struct Input
    {
        Model& model;
        Resource::Ptr& environment;
        Target irradince;
        Target prefilter;
    };

    struct Output
    {
    } output;

    IrradianceConversion(Context& context, const Input& input, int width, int height);

    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnResize(int width, int height) override;
    virtual void OnModifySettings(const Settings & settings) override;

private:
    void DrawIrradianceConvolution();
    void DrawPrefilter();
    void CreateSizeDependentResources();

    Settings m_settings;
    Context& m_context;
    Input m_input;
    int m_width;
    int m_height;
    Resource::Ptr m_sampler;
    Program<CubemapVS, IrradianceConvolutionPS> m_program_irradiance_convolution;
    Program<CubemapVS, PrefilterPS> m_program_prefilter;
    bool is = false;
};
