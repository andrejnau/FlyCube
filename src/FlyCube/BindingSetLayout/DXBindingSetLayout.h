#pragma once
#include "BindingSetLayout/BindingSetLayout.h"

#if defined(_WIN32)
#include <wrl.h>
#else
#include <wsl/wrladapter.h>
#endif

#include <directx/d3d12.h>

using Microsoft::WRL::ComPtr;

class DXDevice;

struct ConstantsLayoutDesc {
    uint32_t root_param_index;
    uint32_t num_constants;
    bool is_compute;
};

struct BindingLayoutDesc {
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
    DXBindingSetLayout(DXDevice& device,
                       const std::vector<BindKey>& bind_keys,
                       const std::vector<BindingConstants>& constants);

    const std::map<D3D12_DESCRIPTOR_HEAP_TYPE, size_t>& GetHeapDescs() const;
    const std::map<BindKey, BindingLayoutDesc>& GetLayout() const;
    const std::map<uint32_t, DescriptorTableDesc>& GetDescriptorTables() const;
    const ComPtr<ID3D12RootSignature>& GetRootSignature() const;
    const std::map<BindKey, ConstantsLayoutDesc>& GetConstantsLayout() const;
    const std::vector<BindingConstants>& GetFallbackConstants() const;

private:
    DXDevice& device_;
    std::map<D3D12_DESCRIPTOR_HEAP_TYPE, size_t> heap_descs_;
    std::map<BindKey, BindingLayoutDesc> layout_;
    std::map<uint32_t, DescriptorTableDesc> descriptor_tables_;
    ComPtr<ID3D12RootSignature> root_signature_;
    std::map<BindKey, ConstantsLayoutDesc> constants_layout_;
    std::vector<BindingConstants> fallback_constants_;
};
