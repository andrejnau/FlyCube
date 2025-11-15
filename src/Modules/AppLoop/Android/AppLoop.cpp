#include "AppLoop/AppLoop.h"

#include "Utilities/Asset.h"

#include <game-activity/native_app_glue/android_native_app_glue.h>

#include <cassert>

namespace {

android_app* g_android_app = nullptr;

struct RendererState {
    std::unique_ptr<AppRenderer> renderer;
    bool initialized = false;
};

AppSize GetSurfaceSize(ANativeWindow* window)
{
    int32_t width = ANativeWindow_getWidth(window);
    int32_t height = ANativeWindow_getHeight(window);
    return AppSize(width, height);
}

void HandleCmd(android_app* app, int32_t cmd)
{
    if (!app->userData) {
        return;
    }

    auto* state = reinterpret_cast<RendererState*>(app->userData);
    switch (cmd) {
    case APP_CMD_INIT_WINDOW: {
        AndroidSurface surface = {
            .window = app->window,
        };
        state->renderer->Init(GetSurfaceSize(app->window), surface);
        state->initialized = true;
        break;
    }
    case APP_CMD_TERM_WINDOW: {
        state->renderer.reset();
        state->initialized = false;
        app->userData = nullptr;
        break;
    }
    default:
        break;
    }
}

void PollEvents()
{
    for (;;) {
        int events = 0;
        android_poll_source* source = nullptr;
        int result = ALooper_pollOnce(/*timeoutMillis=*/0, nullptr, &events, reinterpret_cast<void**>(&source));
        switch (result) {
        case ALOOPER_POLL_TIMEOUT:
        case ALOOPER_POLL_WAKE:
            return;
        case ALOOPER_EVENT_ERROR:
            assert(false);
            break;
        case ALOOPER_POLL_CALLBACK:
            break;
        default:
            if (source) {
                source->process(g_android_app, source);
            }
        }
    }
}

int AndroidLoop(std::unique_ptr<AppRenderer> renderer)
{
    RendererState state = { .renderer = std::move(renderer) };
    g_android_app->userData = &state;
    g_android_app->onAppCmd = HandleCmd;
    while (!g_android_app->destroyRequested) {
        PollEvents();
        if (state.renderer && state.initialized) {
            state.renderer->Render();
        }
    }
    return 0;
}

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
    return AndroidLoop(std::move(renderer));
}

int main(int argc, char* argv[]);

void android_main(android_app* app)
{
    g_android_app = app;
    SetAAssetManager(app->activity->assetManager);
    main(0, nullptr);
    g_android_app = nullptr;
}
