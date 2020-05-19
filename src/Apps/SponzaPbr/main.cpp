#include "Scene.h"
#include <AppBox/AppBox.h>
#include <AppBox/ArgsParser.h>
#include <Context/Context.h>

#ifdef _WIN32
#include <WinConsole/WinConsole.h>
#endif

int main(int argc, char *argv[])
{
#ifdef _WIN32
    WinConsole cmd;
#endif
    Settings settings = {};
    ParseArgs(argc, argv, settings);
    AppBox app("testApp", settings);
    AppRect rect = app.GetAppRect();
    Context context(settings, app.GetWindow());
    decltype(auto) scene = Scene::Create(context, rect.width, rect.height);
    app.SubscribeEvents(scene.get(), scene.get());
    while (!app.PollEvents())
    {
        scene->OnUpdate();
        scene->OnRender();
        app.UpdateFps();
    }
    _exit(0);
}
