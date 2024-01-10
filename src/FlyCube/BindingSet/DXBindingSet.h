#pragma once
#include "BindingSet/BindingSet.h"
#include "Program/ProgramBase.h"

#include <directx/d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXDevice;
class DXBindingSetLayout;
class DXGPUDescriptorPoolRange;

class DXBindingSet : public BindingSet {
public:
    DXBindingSet(DXDevice& device, const std::shared_ptr<DXBindingSetLayout>& layout);

    void WriteBindings(const std::vector<BindingDesc>& bindings) override;

    std::vector<ComPtr<ID3D12DescriptorHeap>> Apply(const ComPtr<ID3D12GraphicsCommandList>& command_list);

private:
    DXDevice& m_device;
    std::shared_ptr<DXBindingSetLayout> m_layout;
    std::map<D3D12_DESCRIPTOR_HEAP_TYPE, std::shared_ptr<DXGPUDescriptorPoolRange>> m_descriptor_ranges;
};
