#include <scene.h>
#include <AppBox/AppBox.h>

int main(void)
{
    return AppBox(tScenes::Create, ApiType::kOpenGL, "[OpenGL] testApp", 1280, 720).Run();
}
