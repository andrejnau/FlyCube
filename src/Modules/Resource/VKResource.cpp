#include "Resource/VKResource.h"
#include <Context/VKContext.h>

VKResource::VKResource(VKContext& context)
    : m_context(context)
{
}

VKResource::~VKResource()
{
    // TODO
    if (!empty)
    {
        empty = true;
        m_context.get().QueryOnDelete(*this);
    }
}

VKResource::VKResource(VKResource&&) = default;
