#pragma once
#include "Shader/ShaderBase.h"
#include <Instance/BaseTypes.h>
#include <vector>

class SpirvShader : public ShaderBase
{
public:
    SpirvShader(const ShaderDesc& desc);
    std::vector<VertexInputDesc> GetInputLayout() const override;
    uint32_t GetResourceStride(const BindKey& bind_key) const override;
    uint32_t GetVertexInputLocation(const std::string& semantic_name) const override;

private:
    void ParseInputLayout();

    std::vector<VertexInputDesc> m_input_layout_desc;
    std::map<std::string, uint32_t> m_locations;
};
