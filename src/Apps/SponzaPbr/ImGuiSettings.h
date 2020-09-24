#pragma once

#include "SponzaSettings.h"
#include <imgui.h>
#include <string>
#include <vector>

class ImGuiSettings
{
public:
    ImGuiSettings(IModifySponzaSettings& listener, SponzaSettings& settings)
        : listener(listener)
        , settings(settings)
    {
    }

    void NewFrame()
    {
        ImGui::NewFrame();
        ImGui::Begin("SponzaSettings");

        if (settings.Has("gpu_name"))
        {
            std::string gpu_name = settings.Get<std::string>("gpu_name");
            if (!gpu_name.empty())
                ImGui::Text("%s", gpu_name.c_str());
        }

        bool has_changed = settings.OnDraw();

        ImGui::End();
        if (has_changed)
            listener.OnModifySponzaSettings(settings);
    }

    void OnKey(int key, int action)
    {
        if (settings.OnKey(key, action))
            listener.OnModifySponzaSettings(settings);
    }

private:
    IModifySponzaSettings& listener;
    SponzaSettings& settings;
};
