#include "ShaderReflection/DXILReflection.h"
#include <Utilities/DXUtility.h>
#include <Utilities/FileUtility.h>
#include <HLSLCompiler/DXCLoader.h>
#include <assert.h>
#include <dxc/DXIL/DxilConstants.h>
#include <dxc/DxilContainer/DxilContainer.h>
#include <dxc/DxilContainer/DxilRuntimeReflection.inl>
#include <dia2.h>
#include <algorithm>

ShaderKind ConvertShaderKind(hlsl::DXIL::ShaderKind kind)
{
    switch (kind)
    {
    case hlsl::DXIL::ShaderKind::Pixel:
        return ShaderKind::kPixel;
    case hlsl::DXIL::ShaderKind::Vertex:
        return ShaderKind::kVertex;
    case hlsl::DXIL::ShaderKind::Geometry:
        return ShaderKind::kGeometry;
    case hlsl::DXIL::ShaderKind::Compute:
        return ShaderKind::kCompute;
    case hlsl::DXIL::ShaderKind::RayGeneration:
        return ShaderKind::kRayGeneration;
    case hlsl::DXIL::ShaderKind::Intersection:
        return ShaderKind::kIntersection;
    case hlsl::DXIL::ShaderKind::AnyHit:
        return ShaderKind::kAnyHit;
    case hlsl::DXIL::ShaderKind::ClosestHit:
        return ShaderKind::kClosestHit;
    case hlsl::DXIL::ShaderKind::Miss:
        return ShaderKind::kMiss;
    case hlsl::DXIL::ShaderKind::Callable:
        return ShaderKind::kCallable;
    case hlsl::DXIL::ShaderKind::Mesh:
        return ShaderKind::kMesh;
    case hlsl::DXIL::ShaderKind::Amplification:
        return ShaderKind::kAmplification;
    }
    assert(false);
    return ShaderKind::kUnknown;
}

ComPtr<IDiaTable> FindTable(ComPtr<IDiaSession> session, const std::wstring& name)
{
    ComPtr<IDiaEnumTables> enum_tables;
    session->getEnumTables(&enum_tables);
    LONG count = 0;
    enum_tables->get_Count(&count);
    for (LONG i = 0; i < count; ++i)
    {
        ULONG fetched = 0;
        ComPtr<IDiaTable> table;
        enum_tables->Next(1, &table, &fetched);
        CComBSTR table_name;
        table->get_name(&table_name);
        if (table_name.m_str == name)
            return table;
    }
    return nullptr;
}

std::string FindStrValue(ComPtr<IDiaTable> table, const std::wstring& name)
{
    LONG count = 0;
    table->get_Count(&count);
    for (LONG i = 0; i < count; ++i)
    {
        CComPtr<IUnknown> item;
        table->Item(i, &item);
        CComPtr<IDiaSymbol> symbol;
        if (FAILED(item.QueryInterface(&symbol)))
            continue;

        CComBSTR item_name;
        symbol->get_name(&item_name);
        if (!item_name || item_name.m_str != name)
            continue;

        VARIANT value = {};
        symbol->get_value(&value);
        if (value.vt == VT_BSTR)
            return wstring_to_utf8(value.bstrVal);
    }
    return "";
}

DXILReflection::DXILReflection(const void* data, size_t size)
{
    DXCLoader loader;
    ComPtr<IDxcLibrary> library;
    loader.CreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library));
    ComPtr<IDxcBlobEncoding> source;
    ASSERT_SUCCEEDED(library->CreateBlobWithEncodingOnHeapCopy(data, size, CP_ACP, &source));
    ComPtr<IDxcContainerReflection> reflection;
    loader.CreateInstance(CLSID_DxcContainerReflection, IID_PPV_ARGS(&reflection));
    ASSERT_SUCCEEDED(reflection->Load(source.Get()));

    ComPtr<IDxcBlob> pdb;
    uint32_t part_count = 0;
    ASSERT_SUCCEEDED(reflection->GetPartCount(&part_count));
    for (uint32_t i = 0; i < part_count; ++i)
    {
        uint32_t kind = 0;
        ASSERT_SUCCEEDED(reflection->GetPartKind(i, &kind));
        if (kind == hlsl::DxilFourCC::DFCC_RuntimeData)
        {
            ParseRuntimeData(reflection, i);
        }
        else if (kind == hlsl::DxilFourCC::DFCC_DXIL)
        {
            ComPtr<ID3D12ShaderReflection> shader_reflection;
            ComPtr<ID3D12LibraryReflection> library_reflection;
            if (SUCCEEDED(reflection->GetPartReflection(i, IID_PPV_ARGS(&shader_reflection))))
            {
                ParseShaderReflection(shader_reflection);
            }
            else if (SUCCEEDED(reflection->GetPartReflection(i, IID_PPV_ARGS(&library_reflection))))
            {
                m_is_library = true;
                ParseLibraryReflection(library_reflection);
            }
        }
        else if (kind == hlsl::DxilFourCC::DFCC_ShaderDebugInfoDXIL)
        {
            reflection->GetPartContent(i, &pdb);
        }
    }

    if (pdb && !m_is_library)
    {
        ParseDebugInfo(loader, pdb);
    }
}

const std::vector<EntryPoint> DXILReflection::GetEntryPoints() const
{
    return m_entry_points;
}

const std::vector<ResourceBindingDesc> DXILReflection::GetBindings() const
{
    return m_bindings;
}

void DXILReflection::ParseRuntimeData(ComPtr<IDxcContainerReflection> reflection, uint32_t idx)
{
    ComPtr<IDxcBlob> part_blob;
    reflection->GetPartContent(idx, &part_blob);
    hlsl::RDAT::DxilRuntimeData context;
    context.InitFromRDAT(part_blob->GetBufferPointer(), part_blob->GetBufferSize());
    hlsl::RDAT::FunctionTableReader* func_table_reader = context.GetFunctionTableReader();
    for (uint32_t j = 0; j < func_table_reader->GetNumFunctions(); ++j)
    {
        hlsl::RDAT::FunctionReader func_reader = func_table_reader->GetItem(j);
        auto kind = func_reader.GetShaderKind();
        m_entry_points.push_back({ func_reader.GetUnmangledName(), ConvertShaderKind(kind) });
    }
}

ViewType GetViewType(const D3D12_SHADER_INPUT_BIND_DESC& bind_desc)
{
    switch (bind_desc.Type)
    {
    case D3D_SIT_CBUFFER:
        return ViewType::kConstantBuffer;
    case D3D_SIT_SAMPLER:
        return ViewType::kSampler;
    case D3D_SIT_TEXTURE:
    case D3D_SIT_STRUCTURED:
    case D3D_SIT_BYTEADDRESS:
    case D3D_SIT_RTACCELERATIONSTRUCTURE:
        return ViewType::kShaderResource;
    case D3D_SIT_UAV_RWSTRUCTURED:
    case D3D_SIT_UAV_RWTYPED:
    case D3D_SIT_UAV_RWBYTEADDRESS:
    case D3D_SIT_UAV_APPEND_STRUCTURED:
    case D3D_SIT_UAV_CONSUME_STRUCTURED:
    case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
        return ViewType::kUnorderedAccess;
    default:
        assert(false);
        return ViewType::kUnknown;
    }
}

ResourceDimension GetResourceDimension(const D3D12_SHADER_INPUT_BIND_DESC& bind_desc)
{
    if (bind_desc.Type == D3D_SIT_RTACCELERATIONSTRUCTURE)
        return ResourceDimension::kRaytracingAccelerationStructure;
    switch (bind_desc.Dimension)
    {
    case D3D_SRV_DIMENSION_UNKNOWN:
        return ResourceDimension::kUnknown;
    case D3D_SRV_DIMENSION_BUFFER:
        return ResourceDimension::kBuffer;
    case D3D_SRV_DIMENSION_TEXTURE1D:
        return ResourceDimension::kTexture1D;
    case D3D_SRV_DIMENSION_TEXTURE1DARRAY:
        return ResourceDimension::kTexture1DArray;
    case D3D_SRV_DIMENSION_TEXTURE2D:
        return ResourceDimension::kTexture2D;
    case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
        return ResourceDimension::kTexture2DArray;
    case D3D_SRV_DIMENSION_TEXTURE2DMS:
        return ResourceDimension::kTexture2DMS;
    case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
        return ResourceDimension::kTexture2DMSArray;
    case D3D_SRV_DIMENSION_TEXTURE3D:
        return ResourceDimension::kTexture3D;
    case D3D_SRV_DIMENSION_TEXTURECUBE:
        return ResourceDimension::kTextureCube;
    case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
        return ResourceDimension::kTextureCubeArray;
    default:
        assert(false);
        return ResourceDimension::kUnknown;
    }
}

ReturnType GetReturnType(const D3D12_SHADER_INPUT_BIND_DESC& bind_desc)
{
    switch (bind_desc.ReturnType)
    {
    case D3D_RETURN_TYPE_FLOAT:
        return ReturnType::kFloat;
    case D3D_RETURN_TYPE_UINT:
        return ReturnType::kUint;
    case D3D_RETURN_TYPE_SINT:
        return ReturnType::kSint;
    default:
        return ReturnType::kUnknown;
    }
}

ResourceBindingDesc GetBindingDesc(const D3D12_SHADER_INPUT_BIND_DESC& bind_desc)
{
    ResourceBindingDesc desc = {};
    desc.name = bind_desc.Name;
    desc.type = GetViewType(bind_desc);
    desc.slot = bind_desc.BindPoint;
    desc.space = bind_desc.Space;
    desc.dimension = GetResourceDimension(bind_desc);
    desc.return_type = GetReturnType(bind_desc);
    return desc;
}

template<typename T, typename U>
std::vector<ResourceBindingDesc> ParseReflection(const T& desc, U* reflection)
{
    std::vector<ResourceBindingDesc> res;
    res.reserve(desc.BoundResources);
    for (uint32_t i = 0; i < desc.BoundResources; ++i)
    {
        D3D12_SHADER_INPUT_BIND_DESC bind_desc = {};
        ASSERT_SUCCEEDED(reflection->GetResourceBindingDesc(i, &bind_desc));
        res.emplace_back(GetBindingDesc(bind_desc));
    }
    return res;
}

void DXILReflection::ParseShaderReflection(ComPtr<ID3D12ShaderReflection> shader_reflection)
{
    D3D12_SHADER_DESC desc = {};
    shader_reflection->GetDesc(&desc);
    hlsl::DXIL::ShaderKind kind = hlsl::GetVersionShaderType(desc.Version);
    m_entry_points.push_back({ "", ConvertShaderKind(kind) });
    m_bindings = ParseReflection(desc, shader_reflection.Get());
}

void DXILReflection::ParseLibraryReflection(ComPtr<ID3D12LibraryReflection> library_reflection)
{
    D3D12_LIBRARY_DESC library_desc = {};
    ASSERT_SUCCEEDED(library_reflection->GetDesc(&library_desc));
    for (uint32_t i = 0; i < library_desc.FunctionCount; ++i)
    {
        ID3D12FunctionReflection* function_reflection = library_reflection->GetFunctionByIndex(i);
        D3D12_FUNCTION_DESC function_desc = {};
        ASSERT_SUCCEEDED(function_reflection->GetDesc(&function_desc));
        auto function_bindings = ParseReflection(function_desc, function_reflection);
        m_bindings.insert(m_bindings.end(), function_bindings.begin(), function_bindings.end());
    }

    std::sort(m_bindings.begin(), m_bindings.end());
    m_bindings.erase(std::unique(m_bindings.begin(), m_bindings.end()), m_bindings.end());
}

void DXILReflection::ParseDebugInfo(DXCLoader& loader, ComPtr<IDxcBlob> pdb)
{
    ComPtr<IDxcLibrary> library;
    ASSERT_SUCCEEDED(loader.CreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library)));
    ComPtr<IStream> stream;
    ASSERT_SUCCEEDED(library->CreateStreamFromBlobReadOnly(pdb.Get(), &stream));

    ComPtr<IDiaDataSource> dia;
    ASSERT_SUCCEEDED(loader.CreateInstance(CLSID_DxcDiaDataSource, IID_PPV_ARGS(&dia)));
    ASSERT_SUCCEEDED(dia->loadDataFromIStream(stream.Get()));
    ComPtr<IDiaSession> session;
    ASSERT_SUCCEEDED(dia->openSession(&session));

    ComPtr<IDiaTable> symbols_table = FindTable(session, L"Symbols");
    assert(m_entry_points.size() == 1);
    m_entry_points.front().name = FindStrValue(symbols_table, L"hlslEntry");
}
