#pragma once
#include "Shader/Shader.h"
#include <Instance/BaseTypes.h>
#include <vector>

class SpirvShader : public Shader
{
public:
    SpirvShader(const ShaderDesc& desc);
    std::vector<VertexInputDesc> GetInputLayout() const override;
    ResourceBindingDesc GetResourceBindingDesc(const std::string& name) const override;
    ShaderType GetType() const override;

    const std::vector<uint32_t>& GetBlob() const;

private:
    ShaderType m_type;
    std::vector<uint32_t> m_blob;
};
