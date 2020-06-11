#pragma once

#include "RenderPass.h"
#include "SponzaSettings.h"
#include <Context/Context.h>
#include <Geometry/Geometry.h>
#include <ProgramRef/SkinningCS.h>

class SkinningPass : public IPass, public IModifySponzaSettings
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
    virtual void OnModifySponzaSettings(const SponzaSettings& settings) override;

private:
    SponzaSettings m_settings;
    Context& m_context;
    Input m_input;
    ProgramHolder<SkinningCS> m_program;
};
