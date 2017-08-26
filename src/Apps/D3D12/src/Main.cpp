#include "DXSample.h"
#include <AppBox/AppBox.h>

int main(void)
{
    return AppBox(DXSample::Create, ApiType::kDX, "[DX12] testApp", 1280, 720).Run();
}
