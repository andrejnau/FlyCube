#pragma once
#include "BindingSetLayout/BindingSetLayout.h"
#include "Program/ProgramBase.h"

#include <directx/d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXDevice;

struct BindingLayout {
    D3D12_DESCRIPTOR_HEAP_TYPE heap_type;
    size_t heap_offset;
};

struct DescriptorTableDesc {
    D3D12_DESCRIPTOR_HEAP_TYPE heap_type;
    size_t heap_offset;
    bool bindless;
    bool is_compute;
};

class DXBindingSetLayout : public BindingSetLayout {
public:
    DXBindingSetLayout(DXDevice& device, const std::vector<BindKey>& descs);

    const std::map<D3D12_DESCRIPTOR_HEAP_TYPE, size_t>& GetHeapDescs() const;
    const std::map<BindKey, BindingLayout>& GetLayout() const;
    const std::map<uint32_t, DescriptorTableDesc>& GetDescriptorTables() const;
    const ComPtr<ID3D12RootSignature>& GetRootSignature() const;

private:
    DXDevice& m_device;
    std::map<D3D12_DESCRIPTOR_HEAP_TYPE, size_t> m_heap_descs;
    std::map<BindKey, BindingLayout> m_layout;
    std::map<uint32_t, DescriptorTableDesc> m_descriptor_tables;
    ComPtr<ID3D12RootSignature> m_root_signature;
};
