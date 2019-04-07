#include "Scene.h"
#include <AppBox/AppBox.h>
#include <Utilities/State.h>
#include <WinConsole/WinConsole.h>

int main(int argc, char *argv[])
{
    WinConsole cmd; 
    return AppBox(argc, argv, Scene::Create, "testApp").Run();
}
