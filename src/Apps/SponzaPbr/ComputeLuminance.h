#pragma once

#include "SponzaSettings.h"
#include "RenderPass.h"
#include <Device/Device.h>
#include <Geometry/Geometry.h>
#include <ProgramRef/HDRLum1DPassCS.h>
#include <ProgramRef/HDRLum2DPassCS.h>
#include <ProgramRef/HDRApplyPS.h>
#include <ProgramRef/HDRApplyVS.h>

class ComputeLuminance : public IPass, public IModifySponzaSettings
{
public:
    struct Input
    {
        std::shared_ptr<Resource>& hdr_res;
        Model& model;
        std::shared_ptr<Resource>& rtv;
        std::shared_ptr<Resource>& dsv;
    };

    struct Output
    {
    } output;

    ComputeLuminance(Device& device, const Input& input, int width, int height);

    virtual void OnUpdate() override;
    virtual void OnRender(CommandListBox& command_list)override;
    virtual void OnResize(int width, int height) override;
    virtual void OnModifySponzaSettings(const SponzaSettings& settings) override;

private:
    void GetLum2DPassCS(CommandListBox& command_list, size_t buf_id, uint32_t thread_group_x, uint32_t thread_group_y);
    void GetLum1DPassCS(CommandListBox& command_list, size_t buf_id, uint32_t input_buffer_size, uint32_t thread_group_x);
    void CreateBuffers();
    uint32_t m_thread_group_x;
    uint32_t m_thread_group_y;

    void Draw(CommandListBox& command_list, size_t buf_id);

    SponzaSettings m_settings;
    Device& m_device;
    Input m_input;
    int m_width;
    int m_height;
    ProgramHolder<HDRLum1DPassCS> m_HDRLum1DPassCS;
    ProgramHolder<HDRLum2DPassCS> m_HDRLum2DPassCS;
    ProgramHolder<HDRApplyPS, HDRApplyVS> m_HDRApply;
    std::vector<std::shared_ptr<Resource>> m_use_res;
};
