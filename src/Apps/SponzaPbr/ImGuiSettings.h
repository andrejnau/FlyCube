#pragma once

#include "SponzaSettings.h"
#include <imgui.h>
#include <string>
#include <vector>

class ImGuiSettings
{
public:
    ImGuiSettings(IModifySponzaSettings& listener)
        : listener(listener)
    {
    }

    void NewFrame()
    {

        ImGui::NewFrame();
        ImGui::Begin("SponzaSettings");

        ImGui::Text("%s", "TODO"); // CurState::Instance().gpu_name.c_str());

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
    SponzaSettings settings;
};
