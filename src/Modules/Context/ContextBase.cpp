#include "Context/ContextBase.h"
#include <Utilities/State.h>

ContextBase::ContextBase(ApiType type, GLFWwindow* window)
    : Context(window)
    , m_instance(CreateInstance(type))
{
    auto adapters = m_instance->EnumerateAdapters();
    uint32_t gpu_index = 0;
    for (auto& adapter : adapters)
    {
        if (CurState::Instance().required_gpu_index != -1 && gpu_index++ != CurState::Instance().required_gpu_index)
            continue;
        CurState::Instance().gpu_name = adapter->GetName();
        m_adapter = std::move(adapter);
        break;
    }

    m_device = m_adapter->CreateDevice();
}
