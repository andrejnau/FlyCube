#pragma once

#include "Settings.h"
#include <imgui.h>
#include <string>
#include <vector>
#include <Utilities/State.h>

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
        }

        add_combo("MSAA", msaa_str, msaa, settings.msaa_count);
        add_checkbox("gamma correction", settings.gamma_correction);
        add_checkbox("use_reinhard_tone_operator", settings.use_reinhard_tone_operator);
        add_checkbox("Tone mapping", settings.use_tone_mapping);
        add_checkbox("use_white_balance", settings.use_white_balance);
        add_checkbox("use_filmic_hdr", settings.use_filmic_hdr);
        add_checkbox("use_avg_lum", settings.use_avg_lum);
        add_checkbox("use model ao", settings.use_ao);
        add_checkbox("use ssao", settings.use_ssao);
        add_checkbox("use shadow", settings.use_shadow);
        add_checkbox("use white ligth", settings.use_white_ligth);
        m_use_IBL = settings.use_IBL_diffuse | settings.use_IBL_specular;
        add_checkbox("use IBL", m_use_IBL, [this]()
        {
            settings.use_IBL_diffuse = settings.use_IBL_specular = m_use_IBL;
        });
        add_checkbox("use IBL diffuse", settings.use_IBL_diffuse);
        add_checkbox("use IBL specular", settings.use_IBL_specular);
        add_checkbox("skip sponza model", settings.skip_sponza_model);
        add_checkbox("only ambientl", settings.only_ambient);
        add_checkbox("light in camera", settings.light_in_camera);
        add_checkbox("additional lights", settings.additional_lights);
        add_checkbox("show_only_albedo", settings.show_only_albedo);
        add_checkbox("show_only_normal", settings.show_only_normal);
        add_checkbox("show_only_roughness", settings.show_only_roughness);
        add_checkbox("show_only_metalness", settings.show_only_metalness);
        add_checkbox("show_only_ao", settings.show_only_ao);
        add_checkbox("use_f0_with_roughness", settings.use_f0_with_roughness);
        add_checkbox("use_flip_normal_y", settings.use_flip_normal_y);
        add_checkbox("use spec ao by ndotv roughness", settings.use_spec_ao_by_ndotv_roughness);
        add_checkbox("irradiance conversion every frame", settings.irradiance_conversion_every_frame);
        add_slider("ambient power", settings.ambient_power, 0.01, 10, true);
        add_slider("light power", settings.light_power, 0.01, 10, true);
        add_slider("Exposure", settings.exposure, 0, 5);
        add_slider("White", settings.white, 0, 5);
        add_checkbox("normal_mapping", settings.normal_mapping).BindKey(GLFW_KEY_N);
        add_checkbox("shadow_discard", settings.shadow_discard).BindKey(GLFW_KEY_J);
        add_checkbox("dynamic_sun_position", settings.dynamic_sun_position).BindKey(GLFW_KEY_SPACE);
    }

    void NewFrame()
    {
        bool has_changed = false;

        ImGui::NewFrame();
        ImGui::Begin("Settings");

        ImGui::Text("%s", CurState::Instance().gpu_name.c_str());

        for (const auto& fn : m_items)
        {
            has_changed |= fn();
        }
        ImGui::End();

        if (has_changed)
            listener.OnModifySettings(settings);
    }

    void OnKey(int key, int action)
    {
        if (action != GLFW_PRESS)
            return;
        bool has_changes = false;
        for (auto &hotkey : m_hotkeys)
        {
            has_changes |= hotkey.OnKey(key);
        }
        if (has_changes)
            listener.OnModifySettings(settings);
    }

private:
    template<typename T>
    void add_combo(const std::string& label, const std::vector<std::string>& items, const std::vector<T>& items_data, T& value)
    {
        int index = 0;
        for (size_t i = 0; i < items_data.size(); ++i)
        {
            if (items_data[i] == value)
                index = static_cast<int>(i);
        }
        struct Capture
        {
            const std::vector<std::string>& items;
        } capture = { items };
        auto fn = [](void* data, int index, const char** text) -> bool
        {
            if (!data || !text || index < 0)
                return false;
            Capture& capture = *static_cast<Capture*>(data);
            if (index >= capture.items.size())
                return false;
            *text = capture.items[index].c_str();
            return true;
        };
        m_items.push_back([index, capture, label, fn, &items, &items_data, &value]() mutable {
            if (ImGui::Combo(label.c_str(), &index, fn, &capture, static_cast<int>(items.size())))
            {
                value = items_data[index];
                return true;
            }
            return false;
        });
    }

    class HotKey
    {
    public:
        HotKey(bool& value)
            : m_value(value)
        {
        }

        void BindKey(int key)
        {
            m_key = key;
        }

        bool OnKey(int key)
        {
            if (m_key == key)
            {
                m_value = !m_value;
                return true;
            }
            return false;
        }

    private:
        int m_key = -1;
        bool& m_value;
    };

    HotKey& add_checkbox(const std::string& label, bool& value)
    {
        m_items.push_back([=, &value]() {
            return ImGui::Checkbox(label.c_str(), &value);
        });
        m_hotkeys.emplace_back(value);
        return m_hotkeys.back();
    }

    template<typename Fn>
    HotKey& add_checkbox(const std::string& label, bool& value, Fn&& fn)
    {
        m_items.push_back([=, &value]() {
            bool res = ImGui::Checkbox(label.c_str(), &value);
            if (res)
                fn();
            return res;
        });
        m_hotkeys.emplace_back(value);
        return m_hotkeys.back();
    }

    void add_slider(const std::string& label, float& value, float min, float max, bool linear = true)
    {
        if (linear)
        {
            m_items.push_back([=, &value]() {
                return ImGui::SliderFloat(label.c_str(), &value, min, max);
            });
        }
        else
        {
            m_items.push_back([=, &value]() {
                return ImGui::SliderFloat(label.c_str(), &value, min, max, "%.3f", 2);
            });
        }
    }

private:
    IModifySettings& listener;
    Settings settings;
    std::vector<uint32_t> msaa = { 1 };
    std::vector<std::string> msaa_str = { "Off" };
    std::vector<std::function<bool(void)>> m_items;
    std::vector<HotKey> m_hotkeys;
    bool m_use_IBL = false;
};
