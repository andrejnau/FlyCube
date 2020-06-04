#pragma once
#include "BindingSet/BindingSet.h"
#include <Program/DXProgram.h>
#include <d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXBindingSet
    : public BindingSet
{
public:
    DXBindingSet(DXProgram& program, bool is_compute,
                 std::map<D3D12_DESCRIPTOR_HEAP_TYPE, std::shared_ptr<DXGPUDescriptorPoolRange>> descriptor_ranges,
                 std::map<std::tuple<ShaderType, D3D12_DESCRIPTOR_RANGE_TYPE, uint32_t /*space*/, bool /*bindless*/>, BindingLayout> binding_layout);
    std::vector<ComPtr<ID3D12DescriptorHeap>> Apply(const ComPtr<ID3D12GraphicsCommandList>& command_list);

private:
    bool m_is_compute;
    std::map<D3D12_DESCRIPTOR_HEAP_TYPE, std::shared_ptr<DXGPUDescriptorPoolRange>> m_descriptor_ranges;
    std::map<std::tuple<ShaderType, D3D12_DESCRIPTOR_RANGE_TYPE, uint32_t /*space*/, bool /*bindless*/>, BindingLayout> m_binding_layout;
};
