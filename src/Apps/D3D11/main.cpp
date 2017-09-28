#include <Scene/SceneBase.h>
#include <AppBox/AppBox.h>
#include <Context/Context.h>

int main(void)
{
    return AppBox(SceneBase::Create, ApiType::kDX11, "[DX11] testApp", 1280, 720).Run();
}
