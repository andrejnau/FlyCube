#include "Scene.h"
#include <AppBox/AppBox.h>
#include <Utilities/State.h>

#ifdef _WIN32
#include <WinConsole/WinConsole.h>
#endif

int main(int argc, char *argv[])
{
#ifdef _WIN32
    WinConsole cmd;
#endif
    return AppBox(argc, argv, Scene::Create, "testApp").Run();
}
