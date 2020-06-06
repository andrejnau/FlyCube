#pragma once
#include "Program/ProgramBase.h"
#include <Shader/Shader.h>
#include <set>
#include <vector>
#include <d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXDevice;
class DXShader;
class DXGPUDescriptorPoolRange;

struct BindingLayout
{
    D3D12_ROOT_PARAMETER_TYPE type;
    uint32_t root_param_index;
    union
    {
        struct
        {
            size_t root_param_heap_offset;
            size_t heap_offset;
            size_t start;
        } table;

        struct
        {
            size_t root_param_num;
        } view;
    };
};

class DXProgram : public ProgramBase
{
public:
    DXProgram(DXDevice& device, const std::vector<std::shared_ptr<Shader>>& shaders);
    bool HasBinding(const BindKey& bind_key) const override;
    std::shared_ptr<BindingSet> CreateBindingSetImpl(const BindingsKey& bindings) override;

    const std::vector<std::shared_ptr<DXShader>>& GetShaders() const;
    const ComPtr<ID3D12RootSignature>& GetRootSignature() const;
    DXDevice& GetDevice();

private:
    size_t HeapSizeByType(D3D12_DESCRIPTOR_HEAP_TYPE type);

    DXDevice& m_device;
    std::vector<std::shared_ptr<DXShader>> m_shaders;

    uint32_t m_num_resources = 0;
    uint32_t m_num_samplers = 0;
    ComPtr<ID3D12RootSignature> m_root_signature;

    std::map<std::tuple<ShaderType, D3D12_DESCRIPTOR_RANGE_TYPE, uint32_t /*space*/, bool /*bindless*/>, BindingLayout> m_binding_layout;
    std::map<std::set<size_t>, std::weak_ptr<DXGPUDescriptorPoolRange>> m_heap_cache;
    struct BindingType
    {
        bool is_bindless = false;
    };
    std::map<BindKey, BindingType> m_binding_type;
    bool m_is_compute = false;
    using BindingsByHeap = std::map<D3D12_DESCRIPTOR_HEAP_TYPE, BindingsKey>;
};
