#include "AppLoop/AppLoop.h"

#include "AppBox/AppBox.h"

#include <cassert>

// static
AppLoop& AppLoop::GetInstance()
{
    static AppLoop instance;
    return instance;
}

AppRenderer& AppLoop::GetRendererImpl()
{
    assert(m_renderer);
    return *m_renderer;
}

int AppLoop::RunImpl(std::unique_ptr<AppRenderer> renderer, int argc, char* argv[])
{
    AppBox app(renderer->GetTitle(), renderer->GetSettings());
    renderer->Init(app.GetAppSize(), app.GetNativeWindow());
    app.SetGpuName(renderer->GetGpuName());
    while (!app.PollEvents()) {
        renderer->Render();
    }
    return 0;
}
