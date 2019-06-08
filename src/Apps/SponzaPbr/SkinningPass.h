#pragma once

#include <Scene/SceneBase.h>
#include "Settings.h"
#include <Context/Context.h>
#include <Geometry/Geometry.h>
#include <ProgramRef/SkinningCS.h>

class SkinningPass : public IPass, public IModifySettings
{
public:
    struct Input
    {
        SceneModels& scene_list;
    };

    struct Output
    {
    } output;

    SkinningPass(Context& context, const Input& input, int width, int height);

    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnModifySettings(const Settings & settings) override;

private:
    Settings m_settings;
    Context& m_context;
    Input m_input;
    Program<SkinningCS> m_program;
};
