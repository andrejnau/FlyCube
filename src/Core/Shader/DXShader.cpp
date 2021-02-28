#include "Shader/DXShader.h"
#include <HLSLCompiler/Compiler.h>
#include <HLSLCompiler/DXReflector.h>
#include <Utilities/DXUtility.h>

DXShader::DXShader(const ShaderDesc& desc)
    : ShaderBase(desc, ShaderBlobType::kDXIL)
{
    if (m_shader_type == ShaderType::kVertex)
        ParseInputLayout();
}

std::vector<VertexInputDesc> DXShader::GetInputLayout() const
{
    return m_input_layout_desc;
}

void DXShader::ParseInputLayout()
{
    ComPtr<ID3D12ShaderReflection> reflector;
    DXReflect(m_blob.data(), m_blob.size(), IID_PPV_ARGS(&reflector));

    D3D12_SHADER_DESC shader_desc = {};
    reflector->GetDesc(&shader_desc);

    for (uint32_t i = 0; i < shader_desc.InputParameters; ++i)
    {
        D3D12_SIGNATURE_PARAMETER_DESC param_desc = {};
        reflector->GetInputParameterDesc(i, &param_desc);

        decltype(auto) layout = m_input_layout_desc.emplace_back();
        std::string semantic_name = param_desc.SemanticName;
        if (param_desc.SemanticIndex)
            semantic_name += std::to_string(param_desc.SemanticIndex);
        layout.slot = layout.location = i;
        m_locations[semantic_name] = layout.location;
        m_semantic_names[layout.location] = semantic_name;

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
        layout.stride = gli::detail::bits_per_pixel(layout.format) / 8;
    }
}

uint32_t DXShader::GetResourceStride(const BindKey& bind_key) const
{
    const std::string& name = m_bindings[m_mapping.at(bind_key)].name;
    D3D12_SHADER_INPUT_BIND_DESC input_bind_desc = {};

    auto impl = [&](auto reflector) -> uint32_t
    {
        if (input_bind_desc.Dimension != D3D_SRV_DIMENSION_BUFFER)
            return 0;
        ID3D12ShaderReflectionConstantBuffer* cbuffer = reflector->GetConstantBufferByName(name.c_str());
        if (!cbuffer)
            return 0;
        D3D12_SHADER_BUFFER_DESC cbdesc = {};
        cbuffer->GetDesc(&cbdesc);
        return cbdesc.Size;
    };

    if (m_shader_type == ShaderType::kLibrary)
    {
        ComPtr<ID3D12LibraryReflection> library_reflector;
        DXReflect(m_blob.data(), m_blob.size(), IID_PPV_ARGS(&library_reflector));
        D3D12_LIBRARY_DESC lib_desc = {};
        library_reflector->GetDesc(&lib_desc);
        for (uint32_t i = 0; i < lib_desc.FunctionCount; ++i)
        {
            auto function_reflector = library_reflector->GetFunctionByIndex(i);
            if (SUCCEEDED(function_reflector->GetResourceBindingDescByName(name.c_str(), &input_bind_desc)))
            {
                return impl(function_reflector);
            }
        }
    }
    else
    {
        ComPtr<ID3D12ShaderReflection> shader_reflector;
        DXReflect(m_blob.data(), m_blob.size(), IID_PPV_ARGS(&shader_reflector));
        ASSERT_SUCCEEDED(shader_reflector->GetResourceBindingDescByName(name.c_str(), &input_bind_desc));
        return impl(shader_reflector);
    }
    assert(false);
    return 0;
}

uint32_t DXShader::GetVertexInputLocation(const std::string& semantic_name) const
{
    return m_locations.at(semantic_name);
}

std::string DXShader::GetSemanticName(uint32_t location) const
{
    return m_semantic_names.at(location);
}
