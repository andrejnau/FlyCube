#include "AppLoop/AppLoop.h"

#include "AppBox/AppBox.h"
#include "AppBox/ArgsParser.h"

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
    Settings settings = ParseArgs(argc, argv);
    AppBox app_box(renderer->GetTitle(), settings);
    renderer->Init(app_box.GetAppSize(), app_box.GetNativeWindow());

    while (!app_box.PollEvents()) {
        renderer->Render();
    }
    return 0;
}
