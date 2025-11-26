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

    void WriteBindings(const std::vector<BindingDesc>& bindings) override;
    void WriteBindingsAndConstants(const std::vector<BindingDesc>& bindings,
                                   const std::vector<BindingConstantsData>& constants) override;

    std::vector<ComPtr<ID3D12DescriptorHeap>> Apply(const ComPtr<ID3D12GraphicsCommandList>& command_list);

private:
    DXDevice& device_;
    std::shared_ptr<DXBindingSetLayout> layout_;
    std::map<D3D12_DESCRIPTOR_HEAP_TYPE, std::shared_ptr<DXGPUDescriptorPoolRange>> descriptor_ranges_;
};
