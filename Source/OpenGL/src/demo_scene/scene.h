#pragma once

#include "shaders.h"
#include "geometry.h"

#include <scenebase.h>
#include <state.h>

#include <simple_camera/camera.h>

#include <chrono>
#include <memory>
#include <random>

#include <SOIL.h>

#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

class tScenes : public ISceneBase
{
public:
    using Ptr = std::unique_ptr<tScenes>;

    tScenes(int width, int height);

    virtual void OnInit() override;
    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnDestroy() override;
    virtual void OnSizeChanged(int width, int height) override;

    Camera & getCamera();

private:

    void GeometryPass();
    void ShadowPass();
    void SSAOPass();
    void SSAOBlurPass();
    void LightPass();
    void RenderLightSource();
    void RenderShadowTexture();
    void RenderCubemap();

    void InitState();
    void InitCamera();
    void InitGBuffer();
    void InitSSAO();
    void InitShadow();
    void InitCubemap();
    void InitMesh();
    GLuint CreateShadowFBO(GLuint shadow_texture);
    GLuint CreateShadowTexture(GLsizei width, GLsizei height);
    GLuint LoadCubemap();

    struct Material
    {
        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;
        float shininess;
    };

    struct Light
    {
        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;
    };

    int width_ = 0;
    int height_ = 0;
    int depth_size_ = 2048;
    float angle_ = 0.0f;
    float model_scale_ = 0.01f;

    GLuint shadow_fbo_ = 0;
    GLuint shadow_texture_ = 0;
    GLuint cubemap_texture_ = 0;

    GLuint ds_fbo_;
    GLuint ssao_fbo_;
    GLuint ssao_blur_fbo_;

    GLuint g_position_;
    GLuint g_normal_;
    GLuint g_ambient_;
    GLuint g_diffuse_;
    GLuint g_specular_;
    GLuint g_ssao_;
    GLuint g_ssao_blur_;
    GLuint g_depth_texture_;
    GLuint noise_texture_;

    glm::vec3 axis_x_ = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 axis_y_ = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 axis_z_ = glm::vec3(0.0f, 0.0f, 1.0f);

    glm::vec3 light_pos_;
    glm::mat4 light_matrix_;
    glm::mat4 light_projection_;
    glm::mat4 light_view_;

    Model model_;
    Model model_sphere_;
    Mesh mesh_square_;
    Mesh mesh_square_shadow_view_;

    std::vector<glm::vec3> ssao_kernel_;

    ShaderGeometryPass shader_geometry_pass_;
    ShaderLightPass shader_light_pass_;
    ShaderSSAOPass shader_ssao_pass_;
    ShaderSSAOBlurPass shader_ssao_blur_pass_;
    ShaderSimpleColor shader_simple_color_;
    ShaderSimpleCubeMap shader_simple_cube_map_;
    ShaderShadowView shader_shadow_view_;
    ShaderDepth shader_depth_;
    Camera camera_;
};