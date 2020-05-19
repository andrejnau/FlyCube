#pragma once

#include "SponzaSettings.h"
#include <Scene/SceneBase.h>
#include <Context/Context.h>
#include <Geometry/Geometry.h>
#include <ProgramRef/GeometryPassPS.h>
#include <ProgramRef/GeometryPassVS.h>

class GeometryPass : public IPass, public IModifySponzaSettings
{
public:
    struct Input
    {
        SceneModels& scene_list;
        Camera& camera;
    };

    struct Output
    {
        std::shared_ptr<Resource> position;
        std::shared_ptr<Resource> normal;
        std::shared_ptr<Resource> albedo;
        std::shared_ptr<Resource> material;
        std::shared_ptr<Resource> dsv;
    } output;

    GeometryPass(Context& context, const Input& input, int width, int height);

    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnResize(int width, int height) override;
    virtual void OnModifySponzaSettings(const SponzaSettings& settings) override;

private:
    Context& m_context;
    Input m_input;
    int m_width;
    int m_height;
    ProgramHolder<GeometryPassPS, GeometryPassVS> m_program;

    void CreateSizeDependentResources();

    std::shared_ptr<Resource> m_sampler;
    SponzaSettings m_settings;
};
