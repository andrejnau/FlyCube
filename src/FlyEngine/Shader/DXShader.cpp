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

ResourceBindingDesc DXShader::GetResourceBindingDesc(const std::string& name) const
{
    D3D12_SHADER_INPUT_BIND_DESC input_bind_desc = {};
    if (m_type == ShaderType::kLibrary)
    {
        ComPtr<ID3D12LibraryReflection> library_reflector;
        DXReflect(m_blob->GetBufferPointer(), m_blob->GetBufferSize(), IID_PPV_ARGS(&library_reflector));
        D3D12_LIBRARY_DESC lib_desc = {};
        library_reflector->GetDesc(&lib_desc);
        for (uint32_t i = 0; i < lib_desc.FunctionCount; ++i)
        {
            auto function_reflector = library_reflector->GetFunctionByIndex(i);
            if (SUCCEEDED(function_reflector->GetResourceBindingDescByName(name.c_str(), &input_bind_desc)))
                break;
        }
    }
    else
    {
        ComPtr<ID3D12ShaderReflection> shader_reflector;
        DXReflect(m_blob->GetBufferPointer(), m_blob->GetBufferSize(), IID_PPV_ARGS(&shader_reflector));
        ASSERT_SUCCEEDED(shader_reflector->GetResourceBindingDescByName(name.c_str(), &input_bind_desc));
    }

    ResourceBindingDesc binding_desc = {};
    switch (input_bind_desc.Dimension)
    {
    case D3D_SRV_DIMENSION_BUFFER:
        binding_desc.dimension = ResourceDimension::kBuffer;
        break;
    case D3D_SRV_DIMENSION_TEXTURE1D:
        binding_desc.dimension = ResourceDimension::kTexture1D;
        break;
    case D3D_SRV_DIMENSION_TEXTURE1DARRAY:
        binding_desc.dimension = ResourceDimension::kTexture1DArray;
        break;
    case D3D_SRV_DIMENSION_TEXTURE2D:
        binding_desc.dimension = ResourceDimension::kTexture2D;
        break;
    case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
        binding_desc.dimension = ResourceDimension::kTexture2DArray;
        break;
    case D3D_SRV_DIMENSION_TEXTURE2DMS:
        binding_desc.dimension = ResourceDimension::kTexture2DMS;
        break;
    case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
        binding_desc.dimension = ResourceDimension::kTexture2DMSArray;
        break;
    case D3D_SRV_DIMENSION_TEXTURE3D:
        binding_desc.dimension = ResourceDimension::kTexture3D;
        break;
    case D3D_SRV_DIMENSION_TEXTURECUBE:
        binding_desc.dimension = ResourceDimension::kTextureCube;
        break;
    case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
        binding_desc.dimension = ResourceDimension::kTextureCubeArray;
        break;
    default:
        break;
    }
    if (input_bind_desc.Type == D3D_SIT_RTACCELERATIONSTRUCTURE)
        binding_desc.dimension = ResourceDimension::kRaytracingAccelerationStructure;
    return binding_desc;
}

ShaderType DXShader::GetType() const
{
    return m_type;
}

ComPtr<ID3DBlob> DXShader::GetBlob() const
{
    return m_blob;
}