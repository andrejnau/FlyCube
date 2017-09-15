#pragma once

#include <ISample/SampleBase.h>
#include <Program/GLProgram.h>
#include <Geometry/GLGeometry.h>

class tScenes : public SampleBase
{
public:
    tScenes();

    static std::unique_ptr<ISample> Create();

    virtual void OnInit(int width, int height) override;
    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnDestroy() override;
    virtual void OnSizeChanged(int width, int height) override;
    virtual glm::mat4 StoreMatrix(const glm::mat4& m) override;

private:
    void GeometryPass();
    void LightPass();

    void InitState();
    void InitCamera();
    void InitGBuffer();

    int load_func_;
    int width_ = 0;
    int height_ = 0;

    GLuint ds_fbo_;

    GLuint g_position_;
    GLuint g_normal_;
    GLuint g_ambient_;
    GLuint g_diffuse_;
    GLuint g_specular_;
    GLuint g_depth_texture_;

    GLProgram shader_geometry_pass_;
    GLProgram shader_light_pass_;

    Model<GLMesh> model_;
    Model<GLMesh> model_square;
};
