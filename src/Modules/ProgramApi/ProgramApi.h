#pragma once
#include <Context/Context.h>
#include <cstddef>
#include <map>
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <Context/Context.h>
#include "ProgramApi/BufferLayout.h"

class ProgramApi
{
public:
    ProgramApi(Context& context);
    std::shared_ptr<Program>& GetProgram();
    Context& GetContext();
    void OnPresent();
    void SetCBufferLayout(const NamedBindKey& bind_key, BufferLayout& buffer_layout);
    void UpdateCBuffers();

private:
    Context& m_context;
    std::map<NamedBindKey, std::reference_wrapper<BufferLayout>> m_cbv_layout;
    struct CBufferData
    {
        std::map<NamedBindKey, std::vector<std::shared_ptr<Resource>>> cbv_buffer;
        std::map<NamedBindKey, size_t> cbv_offset;
    };
    std::vector<CBufferData> m_cbv_data;

protected:
    std::shared_ptr<Program> m_program;
};
