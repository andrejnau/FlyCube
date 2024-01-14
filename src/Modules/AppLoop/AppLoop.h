#pragma once

#include "AppLoop/AppRenderer.h"

#include <memory>

class AppLoop {
public:
    static AppRenderer& GetRenderer()
    {
        return GetInstance().GetRendererImpl();
    }

    static int Run(std::unique_ptr<AppRenderer> renderer, int argc, char* argv[])
    {
        return GetInstance().RunImpl(std::move(renderer), argc, argv);
    }

private:
    static AppLoop& GetInstance();
    AppRenderer& GetRendererImpl();
    int RunImpl(std::unique_ptr<AppRenderer> renderer, int argc, char* argv[]);

    std::unique_ptr<AppRenderer> m_renderer;
};
