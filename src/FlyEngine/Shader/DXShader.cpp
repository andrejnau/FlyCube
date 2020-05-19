#include "Shader/DXShader.h"
#include "Shader/DXCompiler.h"
#include "Shader/DXReflector.h"

DXShader::DXShader(const ShaderDesc& desc)
    : m_type(desc.type)
{
    m_blob = DXCompile(desc);
}

std::vector<VertexInputDesc> DXShader::GetInputLayout() const
{
    ComPtr<ID3D12ShaderReflection> reflector;
    DXReflect(m_blob->GetBufferPointer(), m_blob->GetBufferSize(), IID_PPV_ARGS(&reflector));

    D3D12_SHADER_DESC shader_desc = {};
    reflector->GetDesc(&shader_desc);

    std::vector<VertexInputDesc> input_layout_desc;
    for (uint32_t i = 0; i < shader_desc.InputParameters; ++i)
    {
        D3D12_SIGNATURE_PARAMETER_DESC param_desc = {};
        reflector->GetInputParameterDesc(i, &param_desc);

        decltype(auto) layout = input_layout_desc.emplace_back();
        layout.semantic_name = param_desc.SemanticName;
        if (param_desc.SemanticIndex)
            layout.semantic_name += std::to_string(param_desc.SemanticIndex);
        layout.slot = i;

        if (param_desc.Mask == 1)
        {
            if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                layout.format = gli::format::FORMAT_R32_UINT_PACK32;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                layout.format = gli::format::FORMAT_R32_SINT_PACK32;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                layout.format = gli::format::FORMAT_R32_SFLOAT_PACK32;
        }
        else if (param_desc.Mask <= 3)
        {
            if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                layout.format = gli::format::FORMAT_RG32_UINT_PACK32;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                layout.format = gli::format::FORMAT_RG32_SINT_PACK32;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                layout.format = gli::format::FORMAT_RG32_SFLOAT_PACK32;
        }
        else if (param_desc.Mask <= 7)
        {
            if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                layout.format = gli::format::FORMAT_RGB32_UINT_PACK32;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                layout.format = gli::format::FORMAT_RGB32_SINT_PACK32;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                layout.format = gli::format::FORMAT_RGB32_SFLOAT_PACK32;
        }
        else if (param_desc.Mask <= 15)
        {
            if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                layout.format = gli::format::FORMAT_RGBA32_UINT_PACK32;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                layout.format = gli::format::FORMAT_RGBA32_SINT_PACK32;
            else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                layout.format = gli::format::FORMAT_RGBA32_SFLOAT_PACK32;
        }
    }

    return input_layout_desc;
}

ShaderType DXShader::GetType() const
{
    return m_type;
}

ComPtr<ID3DBlob> DXShader::GetBlob() const
{
    return m_blob;
}