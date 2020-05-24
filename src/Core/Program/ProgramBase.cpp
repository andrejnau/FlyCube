#include "Program/ProgramBase.h"
#include <deque>

std::shared_ptr<BindingSet> ProgramBase::CreateBindingSet(const std::vector<BindingDesc>& bindings)
{
    BindingsKey bindings_key;
    for (auto& desc : bindings)
    {
        auto it = m_bindings_id.find(desc);
        if (it == m_bindings_id.end())
        {
            m_bindings.emplace_back(desc);
            size_t id = m_bindings.size() - 1;
            it = m_bindings_id.emplace(desc, id).first;
        }
        bindings_key.insert(it->second);
    }

    auto it = m_binding_set_cache.find(bindings_key);
    if (it == m_binding_set_cache.end())
    {
        it = m_binding_set_cache.emplace(bindings_key, CreateBindingSetImpl(bindings_key)).first;
    }
    return it->second;
}
