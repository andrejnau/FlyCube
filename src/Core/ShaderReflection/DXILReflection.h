#pragma once
#include "ShaderReflection/ShaderReflection.h"
#include <HLSLCompiler/DXCLoader.h>
#include <d3d12shader.h>

class DXILReflection : public ShaderReflection
{
public:
    DXILReflection(const void* data, size_t size);
    const std::vector<EntryPoint> GetEntryPoints() const override;

private:
    void ParseRuntimeData(ComPtr<IDxcContainerReflection> reflection, uint32_t idx);
    void ParseShaderReflector(ComPtr<ID3D12ShaderReflection> shader_reflector);
    void ParseLibraryReflection(ComPtr<ID3D12LibraryReflection> shader_reflector);
    void ParseDebugInfo(DXCLoader& loader, ComPtr<IDxcBlob> pdb);

    bool m_is_library = false;
    std::vector<EntryPoint> m_entry_points;
};
