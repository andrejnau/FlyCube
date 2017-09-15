#pragma once

#include <cstdint>

#include "ISample.h"
#include <simple_camera/camera.h>
#include <Geometry/Geometry.h>
#include <Program/IShaderBuffer.h>
#include <GLFW/glfw3.h>
#include <state.h>
#include <glm/glm.hpp>
#include <chrono>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

class SampleBase : public ISample
{
public:
    virtual ~SampleBase() {}

    virtual void OnKey(int key, int action) override
    {
        if (action == GLFW_PRESS)
            keys_[key] = true;
        else if (action == GLFW_RELEASE)
            keys_[key] = false;

        if (key == GLFW_KEY_N && action == GLFW_PRESS)
        {
            auto & state = CurState<bool>::Instance().state;
            state["disable_norm"] = !state["disable_norm"];
        }
    }

    virtual void OnMouse(bool first_event, double xpos, double ypos) override
    {
        if (first_event)
        {
            last_x_ = xpos;
            last_y_ = ypos;
        }

        double xoffset = xpos - last_x_;
        double yoffset = last_y_ - ypos;  // Reversed since y-coordinates go from bottom to left

        last_x_ = xpos;
        last_y_ = ypos;

        camera_.ProcessMouseMovement((float)xoffset, (float)yoffset);
    }

    std::vector<int> MapTextures(IMesh& cur_mesh, IShaderBuffer& textures_enables)
    {
        std::vector<int> use_textures(6, -1);

        textures_enables.Uniform("has_diffuseMap") = 0;
        textures_enables.Uniform("has_normalMap") = 0;
        textures_enables.Uniform("has_specularMap") = 0;
        textures_enables.Uniform("has_glossMap") = 0;
        textures_enables.Uniform("has_ambientMap") = 0;
        textures_enables.Uniform("has_alphaMap") = 0;

        for (size_t i = 0; i < cur_mesh.textures.size(); ++i)
        {
            uint32_t texture_slot = 0;
            switch (cur_mesh.textures[i].type)
            {
            case aiTextureType_DIFFUSE:
                texture_slot = 0;
                textures_enables.Uniform("has_diffuseMap") = 1;
                break;
            case aiTextureType_HEIGHT:
            {
                texture_slot = 1;
                auto& state = CurState<bool>::Instance().state;
                if (!state["disable_norm"])
                    textures_enables.Uniform("has_normalMap") = 1;
            } break;
            case aiTextureType_SPECULAR:
                texture_slot = 2;
                textures_enables.Uniform("has_specularMap") = 1;
                break;
            case aiTextureType_SHININESS:
                texture_slot = 3;
                textures_enables.Uniform("has_glossMap") = 1;
                break;
            case aiTextureType_AMBIENT:
                texture_slot = 4;
                textures_enables.Uniform("has_ambientMap") = 1;
                break;
            case aiTextureType_OPACITY:
                texture_slot = 5;
                textures_enables.Uniform("has_alphaMap") = 1;
                break;
            default:
                continue;
            }
            use_textures[texture_slot] = i;
        }
        return use_textures;
    }

    virtual glm::mat4 StoreMatrix(const glm::mat4& x) = 0;

    void UpdateAngle()
    {
        static std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
        std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
        int64_t elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        start = end;

        /*if (m_use_rotare)
            angle += elapsed / 2e6f;*/
    }

    void UpdateCBuffers(IShaderBuffer& constant_buffer_geometry_pass, IShaderBuffer& constant_buffer_light_pass)
    {
        glm::mat4 projection, view, model;
        camera_.GetMatrix(projection, view, model);

        float model_scale_ = 0.01f;
        model = glm::scale(glm::vec3(model_scale_)) * model;

        constant_buffer_geometry_pass.Uniform("model") = StoreMatrix(model);
        constant_buffer_geometry_pass.Uniform("view") = StoreMatrix(view);
        constant_buffer_geometry_pass.Uniform("projection") = StoreMatrix(projection);
        constant_buffer_geometry_pass.Uniform("normalMatrix") = StoreMatrix(model);

        float light_r = 2.5;
        glm::vec3 light_pos_ = glm::vec3(light_r * cos(angle), 25.0f, light_r * sin(angle));

        glm::vec3 cameraPosView = glm::vec3(glm::vec4(camera_.GetCameraPos(), 1.0));
        glm::vec3 lightPosView = glm::vec3(glm::vec4(light_pos_, 1.0));

        constant_buffer_light_pass.Uniform("lightPos") = glm::vec4(lightPosView, 0.0);
        constant_buffer_light_pass.Uniform("viewPos") = glm::vec4(cameraPosView, 0.0);
    }

    void UpdateCameraMovement()
    {
        double currentFrame = glfwGetTime();
        delta_time_ = (float)(currentFrame - last_frame_);
        last_frame_ = currentFrame;

        if (keys_[GLFW_KEY_W])
            camera_.ProcessKeyboard(CameraMovement::kForward, delta_time_);
        if (keys_[GLFW_KEY_S])
            camera_.ProcessKeyboard(CameraMovement::kBackward, delta_time_);
        if (keys_[GLFW_KEY_A])
            camera_.ProcessKeyboard(CameraMovement::kLeft, delta_time_);
        if (keys_[GLFW_KEY_D])
            camera_.ProcessKeyboard(CameraMovement::kRight, delta_time_);
        if (keys_[GLFW_KEY_Q])
            camera_.ProcessKeyboard(CameraMovement::kDown, delta_time_);
        if (keys_[GLFW_KEY_E])
            camera_.ProcessKeyboard(CameraMovement::kUp, delta_time_);
    }

    void SetLight(IShaderBuffer& light_buffer_geometry_pass)
    {
        light_buffer_geometry_pass.Uniform("light_ambient") = glm::vec3(0.2f);
        light_buffer_geometry_pass.Uniform("light_diffuse") = glm::vec3(1.0f);
        light_buffer_geometry_pass.Uniform("light_specular") = glm::vec3(0.5f);
    }

    void SetMaterial(IShaderBuffer& material_buffer_geometry_pass, IMesh& cur_mesh)
    {
        material_buffer_geometry_pass.Uniform("material_ambient") = cur_mesh.material.amb;
        material_buffer_geometry_pass.Uniform("material_diffuse") = cur_mesh.material.dif;
        material_buffer_geometry_pass.Uniform("material_specular") = cur_mesh.material.spec;
        material_buffer_geometry_pass.Uniform("material_shininess") = cur_mesh.material.shininess;
    }

    Camera camera_;
    std::map<int, bool> keys_;
    float last_frame_ = 0.0;
    float delta_time_ = 0.0f;
    double last_x_ = 0.0f;
    double last_y_ = 0.0f;

    float angle = 0.0;
};
