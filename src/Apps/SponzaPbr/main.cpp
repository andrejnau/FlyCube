#include "Scene.h"
#include <AppBox/AppBox.h>
#include <AppBox/ArgsParser.h>
#include <Device/Device.h>

int main(int argc, char *argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppBox app("testApp", settings);
    AppRect rect = app.GetAppRect();
    Context context(settings, app.GetWindow());
    Scene scene(context, rect.width, rect.height);
    app.SubscribeEvents(&scene, &scene);
    while (!app.PollEvents())
    {
        scene.RenderFrame();
        app.UpdateFps();
    }
    _exit(0);
}
