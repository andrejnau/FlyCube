#pragma once
#include <cstdint>
#include <any>
#include <map>
#include <string>
#include <vector>
#include <functional>
#include <GLFW/glfw3.h>

class HotKey
{
public:
    HotKey(std::function<void()> on_key);
    void BindKey(int key);
    bool OnKey(int key);

private:
    int m_key = -1;
    std::function<void()> m_on_key;
};

class SponzaSettings
{
public:
    SponzaSettings();

    bool OnDraw();
    bool OnKey(int key, int action);

    bool Has(const std::string& key) const
    {
        return m_settings.count(key);
    }

    template<typename T>
    T Get(const std::string& key) const
    {
        return std::any_cast<T>(m_settings.at(key));
    }

    template<typename T>
    void Set(const std::string& key, const T& value)
    {
        m_settings[key] = value;
    }

private:
    template<typename T>
    void add_combo(const std::string& label, const std::vector<std::string>& items, const std::vector<T>& items_data, T value);
    HotKey& add_checkbox(const std::string& label, bool value);
    void add_slider_int(const std::string& label, int32_t value, int min, int max);
    void add_slider(const std::string& label, float value, float min, float max, bool linear);

    std::vector<std::function<bool(void)>> m_items;
    std::vector<HotKey> m_hotkeys;
    std::map<std::string, std::any> m_settings;
};

class IModifySponzaSettings
{
public:
    virtual void OnModifySponzaSettings(const SponzaSettings& SponzaSettings) {}
};
