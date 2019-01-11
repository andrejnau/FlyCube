#pragma once

#include "GeometryPass.h"
#include "Settings.h"
#include <Context/DX11Context.h>
#include <Geometry/Geometry.h>
#include <ProgramRef/BRDFPS.h>
#include <ProgramRef/BRDFVS.h>
#include <d3d11.h>
#include <wrl.h>

using namespace Microsoft::WRL;

class BRDFGen : public IPass, public IModifySettings
{
public:
    struct Input
    {
        Model& square_model;
    };

    struct Output
    {
        Resource::Ptr brdf;
    } output;

    BRDFGen(Context& context, const Input& input, int width, int height);

    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnResize(int width, int height) override;
    virtual void OnModifySettings(const Settings & settings) override;

private:
    void DrawBRDF();

    Settings m_settings;
    Context& m_context;
    Input m_input;
    Resource::Ptr m_dsv;
    Program<BRDFVS, BRDFPS> m_program;
    size_t m_size = 512;
    bool is = false;
};
