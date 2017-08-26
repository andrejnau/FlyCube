#include "DXSample.h"
#include <AppBox/AppBox.h>

int main(void)
{
    return AppBox(DXSample(), "[DX11] testApp", 1280, 720).Run();
}
