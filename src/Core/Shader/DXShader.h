#pragma once
#include "Shader/ShaderBase.h"
#include <Instance/BaseTypes.h>
#include <map>
#include <d3dcommon.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXShader : public ShaderBase
{
public:
    DXShader(const ShaderDesc& desc);
    std::vector<VertexInputDesc> GetInputLayout() const override;
    uint32_t GetResourceStride(const BindKey& bind_key) const override;
    uint32_t GetVertexInputLocation(const std::string& semantic_name) const override;

    std::string GetSemanticName(uint32_t location) const;

private:
    void ParseInputLayout();

    std::vector<VertexInputDesc> m_input_layout_desc;
    std::map<std::string, uint32_t> m_locations;
    std::map<uint32_t, std::string> m_semantic_names;
};
