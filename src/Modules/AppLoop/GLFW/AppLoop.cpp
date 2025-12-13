#include "AppLoop/AppLoop.h"

#include "AppBox/AppBox.h"
#include "AppSettings/Settings.h"

#include <cassert>

namespace {

class ResizeHandler : public WindowEvents {
public:
    explicit ResizeHandler(const NativeSurface& surface)
        : surface_(surface)
    {
    }

    void OnResize(uint32_t width, uint32_t height) override
    {
        AppLoop::GetRenderer().Resize(surface_, width, height);
        AppLoop::GetRenderer().Render();
    }

private:
    NativeSurface surface_;
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
    assert(renderer_);
    return *renderer_;
}

int AppLoop::RunImpl(std::unique_ptr<AppRenderer> renderer, int argc, char* argv[])
{
    renderer_ = std::move(renderer);
    AppBox app(renderer_->GetTitle(), renderer_->GetSettings().api_type);
    app.SetGpuName(renderer_->GetGpuName());
    NativeSurface surface = app.GetNativeSurface();
    auto [width, height] = app.GetAppSize();
    renderer_->Init(surface, width, height);
    ResizeHandler resize_handler(surface);
    app.SubscribeEvents(nullptr, &resize_handler);
    while (!app.PollEvents()) {
        renderer_->Render();
    }
    return 0;
}
