#pragma once

#include <Scene/SceneBase.h>
#include <Context/DX11Context.h>
#include <Geometry/DX11Geometry.h>
#include <ProgramRef/HDRLum1DPassCS.h>
#include <ProgramRef/HDRLum2DPassCS.h>
#include <ProgramRef/HDRApplyPS.h>
#include <ProgramRef/HDRApplyVS.h>
#include <wrl.h>

using namespace Microsoft::WRL;

class ComputeLuminance : public IPass, public IModifySettings
{
public:
    struct Input
    {
        Resource::Ptr& hdr_res;
        Model<DX11Mesh>& model;
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
    Resource::Ptr GetLum2DPassCS(uint32_t thread_group_x, uint32_t thread_group_y);
    Resource::Ptr GetLum1DPassCS(Resource::Ptr input, uint32_t input_buffer_size, uint32_t thread_group_x);

    void Draw(Resource::Ptr input);

    Settings m_settings;
    Context& m_context;
    Input m_input;
    int m_width;
    int m_height;
    Program<HDRLum1DPassCS> m_HDRLum1DPassCS;
    Program<HDRLum2DPassCS> m_HDRLum2DPassCS;
    Program<HDRApplyPS, HDRApplyVS> m_HDRApply;
};
