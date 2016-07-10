#pragma once


#define LOG_TAG "FlyCube"
#include <mlogger.h>

#define GLM_FORCE_RADIANS
#include <scenebase.h>
#include "shaders.h"
#include "geometry.h"
#include <state.h>
#include <ctime>
#include <chrono>
#include <memory>
#include <SOIL.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <simple_camera/camera.h>

class tScenes : public SceneBase, public SingleTon < tScenes >
{
public:
    tScenes();

    virtual bool init();
    virtual void draw();
    virtual void resize(int x, int y, int width, int height);
    virtual void destroy();

    GLuint FBODepthCreate(GLuint _Texture_depth);
    GLuint TextureCreateDepth(GLsizei width, GLsizei height);
    GLuint loadCubemap();

    void draw_obj();
    void draw_obj_depth();
    void draw_shadow();
    void draw_cubemap();

    Camera & getCamera();
private:
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

private:
    int m_width = 0;
    int m_height = 0;
    int depth_size = 2048;
    float angle = 0.0f;
    float model_scale = 1.0;

    GLuint depthFBO = 0;
    GLuint depthTexture = 0;
    GLuint c_textureID = 0;

    glm::vec3 axis_x;
    glm::vec3 axis_y;
    glm::vec3 axis_z;

    glm::mat4 lightSpaceMatrix;
    glm::vec3 lightPos;
    glm::mat4 lightProjection;
    glm::mat4 lightView;

    Model modelOfFile;
    Model modelOfFileSphere;

    ShaderLight shaderLight;
    ShaderSimpleColor shaderSimpleColor;
    ShaderSimpleCubeMap shaderSimpleCubeMap;
    ShaderShadowView shaderShadowView;
    ShaderDepth shaderDepth;
    Camera m_camera;
};