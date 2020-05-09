#include "Resource/VKResource.h"

VKResource::VKResource(Destroyer& destroyer)
    : m_destroyer(destroyer)
{
}

VKResource::~VKResource()
{
    // TODO
    if (!empty)
    {
        empty = true;
        m_destroyer.get().QueryOnDelete(std::move(*this));
    }
}

VKResource::VKResource(VKResource&&) = default;
