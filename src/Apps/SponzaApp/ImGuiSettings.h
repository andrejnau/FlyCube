#pragma once

class ImGuiSettings
{
public:
    ImGuiSettings()
    {
        for (uint32_t i = 2; i <= 8; i *= 2)
        {
            msaa_str.push_back("x" + std::to_string(i));
            msaa.push_back(i);
            if (i == settings.msaa_count)
                msaa_idx = msaa.size() - 1;
        }
    }

    void NewFrame(IModifySettings& listener)
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

        if (ImGui::SliderFloat("s_near", &settings.s_near, 0.1f, 8.0f, "%.3f"))
        {
            modify_settings = true;
        }

        if (ImGui::SliderFloat("s_far", &settings.s_far, 0.1f, 1024.0f, "%.3f"))
        {
            modify_settings = true;
        }

        if (ImGui::SliderInt("s_size", &settings.s_size, 512, 4096))
        {
            modify_settings = true;
        }

        if (ImGui::Checkbox("use_shadow", &settings.use_shadow))
        {
            modify_settings = true;
        }

        if (ImGui::Checkbox("Tone mapping", &settings.use_tone_mapping))
        {
            modify_settings = true;
        }

        if (ImGui::Checkbox("use_occlusion", &settings.use_occlusion))
        {
            modify_settings = true;
        }

        if (ImGui::Checkbox("use_blinn", &settings.use_blinn))
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

        if (ImGui::SliderFloat("light_ambient", &settings.light_ambient, 0, 2))
        {
            modify_settings = true;
        }

        if (ImGui::SliderFloat("light_diffuse", &settings.light_diffuse, 0, 2))
        {
            modify_settings = true;
        }

        if (ImGui::SliderFloat("light_specular", &settings.light_specular, 0, 2))
        {
            modify_settings = true;
        }

        ImGui::End();

        if (modify_settings)
            listener.OnModifySettings(settings);
    }

private:
    Settings settings;
    int msaa_idx = 0;
    std::vector<uint32_t> msaa = { 1 };
    std::vector<std::string> msaa_str = { "Off" };
};
