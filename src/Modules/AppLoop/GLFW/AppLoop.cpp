#include "AppLoop/AppLoop.h"

#include "AppBox/AppBox.h"

#include <cassert>

namespace {

class ResizeHandler : public WindowEvents {
public:
    ResizeHandler(void* window)
        : m_window(window)
    {
    }

    void OnResize(int width, int height) override
    {
        AppLoop::GetRenderer().Resize(AppSize(width, height), m_window);
    }

private:
    void* const m_window;
};

} // namespace

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
    m_renderer = std::move(renderer);
    AppBox app(m_renderer->GetTitle(), m_renderer->GetSettings());
    app.SetGpuName(m_renderer->GetGpuName());
    m_renderer->Init(app.GetAppSize(), app.GetNativeWindow());
    ResizeHandler resize_handler(app.GetNativeWindow());
    app.SubscribeEvents(nullptr, &resize_handler);
    while (!app.PollEvents()) {
        m_renderer->Render();
    }
    return 0;
}
