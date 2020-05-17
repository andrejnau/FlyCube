#include "ProgramApi.h"

static size_t GenId()
{
    static size_t id = 0;
    return ++id;
}

ProgramApi::ProgramApi()
    : m_program_id(GenId())
{
}

size_t ProgramApi::GetProgramId() const
{
    return m_program_id;
}

void ProgramApi::SetBindingName(const BindKeyOld& bind_key, const std::string& name)
{
    m_binding_names[bind_key] = name;
}

const std::string& ProgramApi::GetBindingName(const BindKeyOld& bind_key) const
{
    auto it = m_binding_names.find(bind_key);
    if (it == m_binding_names.end())
    {
        static std::string empty;
        return empty;
    }
    return it->second;
}
