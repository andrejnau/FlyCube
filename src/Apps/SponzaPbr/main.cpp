#include "Scene.h"
#include <AppBox/AppBox.h>
#include <AppBox/ArgsParser.h>

int main(int argc, char *argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppBox app("testApp", settings);
    AppRect rect = app.GetAppRect();
    Scene scene(settings, app.GetWindow(), rect.width, rect.height);
    app.SubscribeEvents(&scene, &scene);
    while (!app.PollEvents())
    {
        scene.RenderFrame();
        app.UpdateFps(scene.GetContext().GetGpuName());
    }
    return 0;
}
