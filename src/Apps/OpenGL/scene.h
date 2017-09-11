#pragma once

#include "shaders.h"

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
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

#include <ISample/ISample.h>

#include "GLGeometry.h"

class tScenes : public ISample
{
public:
    tScenes();

    static std::unique_ptr<ISample> Create()
    {
        return std::make_unique<tScenes>();
    }

    static void APIENTRY gl_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *msg, const void *data)
    {
        std::cout << "debug call: " << msg << std::endl;
    }

    virtual void OnInit(int width, int height) override;
    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnDestroy() override;
    virtual void OnSizeChanged(int width, int height) override;

    virtual void OnKey(int key, int action) override;
    virtual void OnMouse(bool first_event, double xpos, double ypos) override;

private:
    void GeometryPass();
    void LightPass();

    void UpdateCameraMovement();

    void InitState();
    void InitCamera();
    void InitGBuffer();

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

    int load_func_;
    int width_ = 0;
    int height_ = 0;
    float angle_ = 0.0f;
    float model_scale_ = 0.01f;

    GLuint ds_fbo_;

    GLuint g_position_;
    GLuint g_normal_;
    GLuint g_ambient_;
    GLuint g_diffuse_;
    GLuint g_specular_;
    GLuint g_depth_texture_;

    glm::vec3 axis_x_ = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 axis_y_ = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 axis_z_ = glm::vec3(0.0f, 0.0f, 1.0f);

    glm::vec3 light_pos_;

    GLShader shader_geometry_pass_;
    GLShader shader_light_pass_;
    Camera camera_;

    Model<GLMesh> model_;
    Model<GLMesh> model_square;

    std::map<int, bool> keys_;
    float last_frame_ = 0.0;
    float delta_time_ = 0.0f;
    double last_x_ = 0.0f;
    double last_y_ = 0.0f;
};
