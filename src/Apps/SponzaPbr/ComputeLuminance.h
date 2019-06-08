#pragma once

#include "Settings.h"
#include <Scene/SceneBase.h>
#include <Context/Context.h>
#include <Geometry/Geometry.h>
#include <ProgramRef/HDRLum1DPassCS.h>
#include <ProgramRef/HDRLum2DPassCS.h>
#include <ProgramRef/HDRApplyPS.h>
#include <ProgramRef/HDRApplyVS.h>

class ComputeLuminance : public IPass, public IModifySettings
{
public:
    struct Input
    {
        Resource::Ptr& hdr_res;
        Model& model;
        Resource::Ptr& rtv;
        Resource::Ptr& dsv;
    };

    struct Output
    {
    } output;

    ComputeLuminance(Context& context, const Input& input, int width, int height);

    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnResize(int width, int height) override;
    virtual void OnModifySettings(const Settings & settings) override;

private:
    void GetLum2DPassCS(size_t buf_id, uint32_t thread_group_x, uint32_t thread_group_y);
    void GetLum1DPassCS(size_t buf_id, uint32_t input_buffer_size, uint32_t thread_group_x);
    void CreateBuffers();
    uint32_t m_thread_group_x;
    uint32_t m_thread_group_y;

    void Draw(size_t buf_id);

    Settings m_settings;
    Context& m_context;
    Input m_input;
    int m_width;
    int m_height;
    Program<HDRLum1DPassCS> m_HDRLum1DPassCS;
    Program<HDRLum2DPassCS> m_HDRLum2DPassCS;
    Program<HDRApplyPS, HDRApplyVS> m_HDRApply;
    std::vector<Resource::Ptr> m_use_res;
};
