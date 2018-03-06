#pragma once

#include <Context/DX12Context.h>
#include "Program/ProgramApi.h"
#include <algorithm>
#include <deque>
#include <array>
#include <set>

#include "Context/DescriptorPool.h"

class DX12ProgramApi : public ProgramApi
{
public:
    DX12ProgramApi(DX12Context& context);

    virtual void SetMaxEvents(size_t count) override;
    virtual void UseProgram() override;
    virtual void ApplyBindings() override;
    virtual void OnCompileShader(ShaderType type, const ComPtr<ID3DBlob>& blob) override;
    virtual void AttachSRV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& res) override;
    virtual void AttachUAV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& res) override;
    virtual void AttachCBuffer(ShaderType type, UINT slot, BufferLayout& buffer) override;
    virtual void AttachSampler(ShaderType type, uint32_t slot, const SamplerDesc& desc) override;

    void OnPresent();
    void OMSetRenderTargets(std::vector<Resource::Ptr> rtv, Resource::Ptr dsv);

private:
    void AttachCBV(ShaderType type, uint32_t slot, const ComPtr<ID3D12Resource>& res);
    DescriptorHeapRange CreateSrv(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires);
    DescriptorHeapRange CreateUAV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires);
    DescriptorHeapRange CreateCBV(ShaderType type, uint32_t slot, const ComPtr<ID3D12Resource>& res);
    DescriptorHeapRange CreateSampler(ShaderType type, uint32_t slot, const SamplerDesc& desc);
    DescriptorHeapRange CreateRTV(uint32_t slot, const Resource::Ptr& ires);
    DescriptorHeapRange CreateDSV(const Resource::Ptr& ires);
    ComPtr<ID3D12Resource> CreateCBuffer(size_t buffer_size);

    void SetRootSignature(ID3D12RootSignature* pRootSignature);
    void SetRootDescriptorTable(UINT RootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor);
    std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputLayout(ComPtr<ID3D12ShaderReflection> reflector);
    void CreateGraphicsPSO();
    void CreateComputePSO();
    void ParseShaders();

private:
    size_t m_num_cbv = 0;
    size_t m_num_srv = 0;
    size_t m_num_uav = 0;
    size_t m_num_rtv = 0;
    size_t m_num_sampler = 0;
    std::map<std::tuple<ShaderType, ResourceType, uint32_t>, DescriptorHeapRange> m_heap_ranges;
    std::map<ShaderType, ComPtr<ID3DBlob>> m_blob_map;

    D3D12_ROOT_DESCRIPTOR_TABLE DescriptorTable;
    D3D12_ROOT_CONSTANTS Constants;
    D3D12_ROOT_DESCRIPTOR Descriptor;

    struct BindingLayout
    {
        D3D12_ROOT_PARAMETER_TYPE type;
        size_t root_param_index;
        union
        {
            struct
            {
                size_t root_param_heap_offset;
                size_t heap_offset;
            } table;

            struct
            {
                size_t root_param_num;
            } view;
        };
    };

    std::map<std::tuple<ShaderType, D3D12_DESCRIPTOR_RANGE_TYPE>, BindingLayout> m_binding_layout;
    std::map<std::tuple<ShaderType, size_t>, std::reference_wrapper<BufferLayout>> m_cbv_layout;
    PerFrameData<std::map<std::tuple<ShaderType, size_t>, std::vector<ComPtr<ID3D12Resource>>>> m_cbv_buffer;
    PerFrameData<std::map<std::tuple<ShaderType, size_t>, size_t>> m_cbv_offset;
    ComPtr<ID3D12RootSignature> m_root_signature;
    bool m_changed_pso_desc = false;
    bool m_changed_binding = false;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC m_pso_desc = {};
    D3D12_COMPUTE_PIPELINE_STATE_DESC m_compute_pso_desc = {};
    ComPtr<ID3D12PipelineState> m_pso;

    const bool m_use_cbv_table = false;

    DX12Context& m_context;
    size_t m_program_id;
};
