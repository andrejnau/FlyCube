#include "Context/Context.h"

#ifdef BUILD_DX11
#include <DX11Context/DX11Context.h>
#endif

#ifdef BUILD_OPENGL
#include <GLContext/GLContext.h>
#endif

Context::Ptr CreareContext(ApiType api_type, int width, int height)
{
    return CreareDX11Context(width, height);
}
