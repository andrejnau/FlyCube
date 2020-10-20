#pragma once

#include "GeometryPass.h"
#include "SponzaSettings.h"
#include <Device/Device.h>
#include <Geometry/Geometry.h>
#include <ProgramRef/CubemapVS.h>
#include <ProgramRef/IrradianceConvolutionPS.h>
#include <ProgramRef/PrefilterPS.h>

class IrradianceConversion : public IPass, public IModifySponzaSettings
{
public:
    struct Target
    {
        std::shared_ptr<Resource>& res;
        std::shared_ptr<Resource>& dsv;
        size_t layer;
        size_t size;
    };

    struct Input
    {
        Model& model;
        std::shared_ptr<Resource>& environment;
        Target irradince;
        Target prefilter;
    };

    struct Output
    {
    } output;

    IrradianceConversion(RenderDevice& device, const Input& input);

    virtual void OnUpdate() override;
    virtual void OnRender(RenderCommandList& command_list)override;
    virtual void OnModifySponzaSettings(const SponzaSettings& settings) override;

private:
    void DrawIrradianceConvolution(RenderCommandList& command_list);
    void DrawPrefilter(RenderCommandList& command_list);

    SponzaSettings m_settings;
    RenderDevice& m_device;
    Input m_input;
    std::shared_ptr<Resource> m_sampler;
    ProgramHolder<CubemapVS, IrradianceConvolutionPS> m_program_irradiance_convolution;
    ProgramHolder<CubemapVS, PrefilterPS> m_program_prefilter;
    bool is = false;
};
