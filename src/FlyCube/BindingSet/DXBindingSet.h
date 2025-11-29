#pragma once
#include "BindingSet/BindingSetBase.h"

#if defined(_WIN32)
#include <wrl.h>
#else
#include <wsl/wrladapter.h>
#endif

#include <directx/d3d12.h>

using Microsoft::WRL::ComPtr;

class DXDevice;
class DXBindingSetLayout;
class DXGPUDescriptorPoolRange;

class DXBindingSet : public BindingSetBase {
public:
    DXBindingSet(DXDevice& device, const std::shared_ptr<DXBindingSetLayout>& layout);

    void WriteBindings(const WriteBindingsDesc& desc) override;

    std::vector<ComPtr<ID3D12DescriptorHeap>> Apply(const ComPtr<ID3D12GraphicsCommandList>& command_list);

private:
    void WriteDescriptor(const BindingDesc& binding);

    DXDevice& device_;
    std::shared_ptr<DXBindingSetLayout> layout_;
    std::map<D3D12_DESCRIPTOR_HEAP_TYPE, std::shared_ptr<DXGPUDescriptorPoolRange>> descriptor_ranges_;
    std::vector<uint32_t> constants_data_;
    std::map<BindKey, uint64_t> constants_data_offsets_;
};
