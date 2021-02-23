#include "ShaderReflection/DXILReflection.h"
#include <Utilities/DXUtility.h>
#include <Utilities/FileUtility.h>
#include <HLSLCompiler/DXCLoader.h>
#include <assert.h>
#include <dxc/DXIL/DxilConstants.h>
#include <dxc/DxilContainer/DxilContainer.h>
#include <dxc/DxilContainer/DxilRuntimeReflection.inl>
#include <dia2.h>

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
            ComPtr<ID3D12ShaderReflection> shader_reflector;
            ComPtr<ID3D12LibraryReflection> library_reflection;
            if (SUCCEEDED(reflection->GetPartReflection(i, IID_PPV_ARGS(&shader_reflector))))
            {
                ParseShaderReflector(shader_reflector);
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

void DXILReflection::ParseRuntimeData(ComPtr<IDxcContainerReflection> reflection, uint32_t idx)
{
    ComPtr<IDxcBlob> part_blob;
    reflection->GetPartContent(idx, &part_blob);
    hlsl::RDAT::DxilRuntimeData context;
    context.InitFromRDAT(part_blob->GetBufferPointer(), part_blob->GetBufferSize());
    FunctionTableReader* func_table_reader = context.GetFunctionTableReader();
    for (uint32_t j = 0; j < func_table_reader->GetNumFunctions(); ++j)
    {
        FunctionReader func_reader = func_table_reader->GetItem(j);
        auto kind = func_reader.GetShaderKind();
        m_entry_points.push_back({ func_reader.GetUnmangledName(), ConvertShaderKind(kind) });
    }
}

void DXILReflection::ParseShaderReflector(ComPtr<ID3D12ShaderReflection> shader_reflector)
{
    D3D12_SHADER_DESC desc = {};
    shader_reflector->GetDesc(&desc);
    DXIL::ShaderKind kind = GetVersionShaderType(desc.Version);
    m_entry_points.push_back({ "", ConvertShaderKind(kind) });
}

void DXILReflection::ParseLibraryReflection(ComPtr<ID3D12LibraryReflection> shader_reflector)
{

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
