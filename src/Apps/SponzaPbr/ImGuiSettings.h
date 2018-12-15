#pragma once

#include "Settings.h"
#include <imgui.h>
#include <string>
#include <vector>

class ImGuiSettings
{
public:
    ImGuiSettings(IModifySettings& listener)
        : listener(listener)
    {
        for (uint32_t i = 2; i <= 8; i *= 2)
        {
            msaa_str.push_back("x" + std::to_string(i));
            msaa.push_back(i);
            if (i == settings.msaa_count)
                msaa_idx = msaa.size() - 1;
        }
    }

    void NewFrame()
    {
        bool modify_settings = false;
        ImGui::NewFrame();

        ImGui::Begin("Settings");

        static auto fn = [](void* data, int idx, const char** out_text) -> bool
        {
            if (!data || !out_text)
                return false;
            const auto& self = *(ImGuiSettings*)data;
            *out_text = self.msaa_str[idx].c_str();
            return true;
        };

        if (ImGui::Combo("MSAA", &msaa_idx, fn, this, msaa_str.size()))
        {
            settings.msaa_count = msaa[msaa_idx];
            modify_settings = true;
        }

        if (ImGui::Checkbox("Tone mapping", &settings.use_tone_mapping))
        {
            modify_settings = true;
        }

        if (ImGui::Checkbox("use simple hdr", &settings.use_simple_hdr))
        {
            modify_settings = true;
        }

        if (ImGui::Checkbox("use_occlusion", &settings.use_occlusion))
        {
            modify_settings = true;
        }

        if (ImGui::Checkbox("use white ligth", &settings.use_white_ligth))
        {
            modify_settings = true;
        }

        if (ImGui::Checkbox("enable diffuse for metal", &settings.enable_diffuse_for_metal))
        {
            modify_settings = true;
        }

        if (ImGui::SliderInt("ssao_scale", &settings.ssao_scale, 1, 8))
        {
            modify_settings = true;
        }

        if (ImGui::SliderFloat("Exposure", &settings.Exposure, 0, 2))
        {
            modify_settings = true;
        }

        if (ImGui::SliderFloat("White", &settings.White, 0, 5))
        {
            modify_settings = true;
        }

        ImGui::End();

        if (modify_settings)
            listener.OnModifySettings(settings);
    }

    void OnKey(int key, int action)
    {
        if (key == GLFW_KEY_O && action == GLFW_PRESS)
            settings.use_occlusion ^= true;
        listener.OnModifySettings(settings);
    }

private:
    IModifySettings& listener;
    Settings settings;
    int msaa_idx = 0;
    std::vector<uint32_t> msaa = { 1 };
    std::vector<std::string> msaa_str = { "Off" };
};
