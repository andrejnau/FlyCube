#include "AppLoop/AppLoop.h"

#include "AppBox/AppBox.h"

#include <cassert>

namespace {

class ResizeHandler : public WindowEvents {
public:
    ResizeHandler(const NativeSurface& surface)
        : surface_(surface)
    {
    }

    void OnResize(int width, int height) override
    {
        AppLoop::GetRenderer().Resize(AppSize(width, height), surface_);
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
    AppBox app(renderer_->GetTitle(), renderer_->GetSettings());
    app.SetGpuName(renderer_->GetGpuName());
    NativeSurface surface = app.GetNativeSurface();
    renderer_->Init(app.GetAppSize(), surface);
    ResizeHandler resize_handler(surface);
    app.SubscribeEvents(nullptr, &resize_handler);
    while (!app.PollEvents()) {
        renderer_->Render();
    }
    return 0;
}
