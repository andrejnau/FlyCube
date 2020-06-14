#pragma once

#include "GeometryPass.h"
#include "SponzaSettings.h"
#include <Device/Device.h>
#include <Geometry/Geometry.h>
#include <ProgramRef/BRDFPS.h>
#include <ProgramRef/BRDFVS.h>

class BRDFGen : public IPass, public IModifySponzaSettings
{
public:
    struct Input
    {
        Model& square_model;
    };

    struct Output
    {
        std::shared_ptr<Resource> brdf;
    } output;

    BRDFGen(Device& device, const Input& input);

    virtual void OnUpdate() override;
    virtual void OnRender(CommandListBox& command_list)override;
    virtual void OnResize(int width, int height) override;
    virtual void OnModifySponzaSettings(const SponzaSettings& settings) override;

private:
    void DrawBRDF(CommandListBox& command_list);

    SponzaSettings m_settings;
    Device& m_device;
    Input m_input;
    std::shared_ptr<Resource> m_dsv;
    ProgramHolder<BRDFVS, BRDFPS> m_program;
    size_t m_size = 512;
    bool is = false;
};
