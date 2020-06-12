#pragma once

#include "GeometryPass.h"
#include "SponzaSettings.h"
#include <Device/Device.h>
#include <Geometry/Geometry.h>
#include <ProgramRef/CubemapVS.h>
#include <ProgramRef/Equirectangular2CubemapPS.h>
#include <ProgramRef/IrradianceConvolutionPS.h>
#include <ProgramRef/PrefilterPS.h>
#include <ProgramRef/DownSampleCS.h>

class Equirectangular2Cubemap : public IPass, public IModifySponzaSettings
{
public:
    struct Input
    {
        Model& model;
        std::shared_ptr<Resource>& hdr;
    };

    struct Output
    {
        std::shared_ptr<Resource> environment;
    } output;

    Equirectangular2Cubemap(Device& device, const Input& input, int width, int height);

    virtual void OnUpdate() override;
    virtual void OnRender(CommandListBox& command_list)override;
    virtual void OnResize(int width, int height) override;
    virtual void OnModifySponzaSettings(const SponzaSettings& settings) override;

private:
    void DrawEquirectangular2Cubemap(CommandListBox& command_list);
    void CreateSizeDependentResources();

    SponzaSettings m_settings;
    Device& m_device;
    Input m_input;
    int m_width;
    int m_height;
    std::shared_ptr<Resource> m_sampler;
    std::shared_ptr<Resource> m_dsv;
    ProgramHolder<CubemapVS, Equirectangular2CubemapPS> m_program_equirectangular2cubemap;
    ProgramHolder<DownSampleCS> m_program_downsample;
    size_t m_texture_size = 512;
    size_t m_texture_mips = 0;
    bool is = false;
};
