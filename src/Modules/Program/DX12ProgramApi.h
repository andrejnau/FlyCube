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
    void UseProgram() override;
    virtual void OnCompileShader(ShaderType type, const ComPtr<ID3DBlob>& blob) override;
    virtual void AttachSRV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& res) override;
    virtual void AttachUAV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& res) override;
    virtual void AttachCBuffer(ShaderType type, uint32_t slot, const Resource::Ptr& ires) override;
    virtual void AttachSampler(ShaderType type, uint32_t slot, const SamplerDesc& desc) override;
    virtual void UpdateData(ShaderType type, UINT slot, const Resource::Ptr& ires, const void* ptr) override;
    virtual Context& GetContext() override;

    void ApplyBindings();
    void OnPresent();
    void OMSetRenderTargets(std::vector<Resource::Ptr> rtv, Resource::Ptr dsv);

private:
    DescriptorHeapRange CreateSrv(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires);
    DescriptorHeapRange CreateUAV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires);
    DescriptorHeapRange CreateCBV(ShaderType type, uint32_t slot, const Resource::Ptr& ires);
    DescriptorHeapRange CreateSampler(ShaderType type, uint32_t slot, const SamplerDesc& desc);

    void SetRootSignature(ID3D12RootSignature* pRootSignature);
    void SetRootDescriptorTable(UINT RootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor);
    std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputLayout(ComPtr<ID3D12ShaderReflection> reflector);
    void CreateGraphicsPSO();
    void CreateComputePSO();

    void ParseShaders();

    void CreateRTV(uint32_t slot, const Resource::Ptr& ires);

    void CreateDSV(const Resource::Ptr& ires);

private:

    size_t m_num_cbv = 0;
    size_t m_num_srv = 0;
    size_t m_num_uav = 0;
    size_t m_num_rtv = 0;
    size_t m_num_sampler = 0;

    std::map<std::tuple<ShaderType, ResourceType, uint32_t>, DescriptorHeapRange> m_ranges;

    std::map<ShaderType, ComPtr<ID3DBlob>> m_blob_map;

    std::map<ShaderType, std::map<D3D12_DESCRIPTOR_RANGE_TYPE, size_t>> m_heap_offset_map;
    std::map<ShaderType, std::map<D3D12_DESCRIPTOR_RANGE_TYPE, size_t>> m_root_param_map;
    std::map<ShaderType, std::map<D3D12_DESCRIPTOR_RANGE_TYPE, size_t>> m_root_param_start_heap_map;

    bool is_first_draw;
    bool change_pso_desc = false;

    ComPtr<ID3D12DescriptorHeap> rtv_heap;
    ComPtr<ID3D12DescriptorHeap> dsv_heap;
    ComPtr<ID3D12RootSignature> rootSignature;

    std::map<std::tuple<ShaderType, UINT>, size_t>  ver;
    size_t draw_offset = 0;

    size_t used_rtv = 0;
    bool use_dsv = false;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    D3D12_COMPUTE_PIPELINE_STATE_DESC compute_pso = {};
    ComPtr<ID3D12PipelineState> pipelineStateObject; // pso containing a pipeline state

    DX12Context& m_context;
    size_t m_program_id;
};
