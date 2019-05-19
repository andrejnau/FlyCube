#pragma once

#include "GeometryPass.h"
#include "Settings.h"
#include <Context/Context.h>
#include <Geometry/Geometry.h>
#include <ProgramRef/BackgroundPS.h>
#include <ProgramRef/BackgroundVS.h>
#include <d3d11.h>
#include <wrl.h>

using namespace Microsoft::WRL;

class BackgroundPass : public IPass, public IModifySettings
{
public:
    struct Input
    {
        Model& model;
        Camera& camera;
        Resource::Ptr& environment;
        Resource::Ptr& rtv;
        Resource::Ptr& dsv;
    };

    struct Output
    {
        Resource::Ptr environment;
        Resource::Ptr irradince;
    } output;

    BackgroundPass(Context& context, const Input& input, int width, int height);

    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnResize(int width, int height) override;
    virtual void OnModifySettings(const Settings & settings) override;

private:
    Settings m_settings;
    Context& m_context;
    Input m_input;
    int m_width;
    int m_height;
    Resource::Ptr m_sampler;
    Program<BackgroundVS, BackgroundPS> m_program;
};
