#pragma once

#include <cstdint>

#include "ISample.h"
#include <simple_camera/camera.h>
#include <Geometry/Geometry.h>
#include <Program/IShaderBuffer.h>
#include <GLFW/glfw3.h>
#include <state.h>

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

    Camera camera_;
    std::map<int, bool> keys_;
    float last_frame_ = 0.0;
    float delta_time_ = 0.0f;
    double last_x_ = 0.0f;
    double last_y_ = 0.0f;
};
