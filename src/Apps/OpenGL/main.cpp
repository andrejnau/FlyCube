#include "GLScene.h"
#include <AppBox/AppBox.h>

int main(void)
{
    return AppBox(GLScene::Create, ApiType::kOpenGL, "[OpenGL] testApp", 1280, 720).Run();
}
