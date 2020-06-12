#include "SponzaSettings.h"
#include <cmath>
#include <imgui.h>

HotKey::HotKey(std::function<void()> on_key)
    : m_on_key(on_key)
{
}

void HotKey::BindKey(int key)
{
    m_key = key;
}

bool HotKey::OnKey(int key)
{
    if (m_key == key)
    {
        m_on_key();
        return true;
    }
    return false;
}

template<typename T>
void SponzaSettings::add_combo(const std::string& label, const std::vector<std::string>& items, const std::vector<T>& items_data, T value)
{
    m_settings[label] = value;
    int index = 0;
    for (size_t i = 0; i < items_data.size(); ++i)
    {
        if (items_data[i] == value)
            index = static_cast<int>(i);
    }
    struct Capture
    {
        std::vector<std::string> items;
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
    m_items.push_back([index, capture, label, fn, items, items_data, this]() mutable
    {
        if (ImGui::Combo(label.c_str(), &index, fn, &capture, static_cast<int>(items.size())))
        {
            m_settings[label] = items_data[index];
            return true;
        }
        return false;
    });
}

HotKey& SponzaSettings::add_checkbox(const std::string& label, bool value)
{
    m_settings[label] = value;
    m_items.push_back([this, label, value]() mutable
    {
        auto res = ImGui::Checkbox(label.c_str(), &value);
        m_settings[label] = value;
        return res;
    });
    m_hotkeys.emplace_back([this, label]
    {
        m_settings[label] = !std::any_cast<bool>(m_settings[label]);
    });
    return m_hotkeys.back();
}

void SponzaSettings::add_slider_int(const std::string& label, int32_t value, int min, int max)
{
    m_settings[label] = value;
    m_items.push_back([this, label, value, min, max]() mutable
    {
        auto res = ImGui::SliderInt(label.c_str(), &value, min, max);
        m_settings[label] = value;
        return res;
    });
}

void SponzaSettings::add_slider(const std::string& label, float value, float min, float max, bool linear)
{
    m_settings[label] = value;
    if (linear)
    {
        m_items.push_back([this, label, value, min, max]() mutable
        {
            auto res = ImGui::SliderFloat(label.c_str(), &value, min, max);
            m_settings[label] = value;
            return res;
        });
    }
    else
    {
        m_items.push_back([this, label, value, min, max]() mutable
        {
            auto res = ImGui::SliderFloat(label.c_str(), &value, min, max, "%.3f", 2);
            m_settings[label] = value;
            return res;
        });
    }
}

SponzaSettings::SponzaSettings()
{
    std::vector<std::string> msaa_str = { "Off" };
    std::vector<uint32_t> msaa = { 1 };

    for (uint32_t i = 2; i <= 8; i *= 2)
    {
        msaa_str.push_back("x" + std::to_string(i));
        msaa.push_back(i);
    }

    add_combo("msaa_count", msaa_str, msaa, 1u);
    add_checkbox("gamma_correction", true);
    add_checkbox("use_reinhard_tone_operator", false);
    add_checkbox("use_tone_mapping", true);
    add_checkbox("use_white_balance", true);
    add_checkbox("use_filmic_hdr", false);
    add_checkbox("use_avg_lum", false);
    add_checkbox("use_ao", false);
    add_checkbox("use_ssao", true);
    add_checkbox("use_rtao", false);
    add_checkbox("use_ao_blur", true);
    add_slider_int("rtao_num_rays", 32, 1, 128);
    add_slider("ao_radius", 0.05, 0.01, 5, false);
    add_checkbox("use_alpha_test", true);
    add_checkbox("use_shadow", true);
    add_checkbox("use_white_ligth", true);
    add_checkbox("use_IBL_diffuse", true);
    add_checkbox("use_IBL_specular", true);
    add_checkbox("skip_sponza_model", false);
    add_checkbox("only_ambient", false);
    add_checkbox("light_in_camera", false);
    add_checkbox("additional_lights", false);
    add_checkbox("show_only_ao", false);
    add_checkbox("show_only_position", false);
    add_checkbox("show_only_albedo", false);
    add_checkbox("show_only_normal", false);
    add_checkbox("show_only_roughness", false);
    add_checkbox("show_only_metalness", false);
    add_checkbox("use_f0_with_roughness", false);
    add_checkbox("use_flip_normal_y", false);
    add_checkbox("use_spec_ao_by_ndotv_roughness", true);
    add_checkbox("irradiance_conversion_every_frame", false);
    add_slider("ambient_power", 1.0, 0.01, 10, true);
    add_slider("light_power", acos(-1.0), 0.01, 10, true);
    add_slider("exposure", 1, 0, 5, false);
    add_slider("white", 1, 0, 5, false);
    add_checkbox("normal_mapping", true).BindKey(GLFW_KEY_N);
    add_checkbox("shadow_discard", true).BindKey(GLFW_KEY_J);
    add_checkbox("dynamic_sun_position", false).BindKey(GLFW_KEY_SPACE);
    add_slider("s_near", 0.1, 0.01, 10, true);
    add_slider("s_far", 1024.0, 128, 4096, true);
    add_slider("s_size", 3072, 128, 8192, true);
}

bool SponzaSettings::OnDraw()
{
    bool has_changed = false;
    for (const auto& fn : m_items)
    {
        has_changed |= fn();
    }
    return has_changed;
}

bool SponzaSettings::OnKey(int key, int action)
{
    if (action != GLFW_PRESS)
        return false;
    bool has_changes = false;
    for (auto& hotkey : m_hotkeys)
    {
        has_changes |= hotkey.OnKey(key);
    }
    return has_changes;
}
