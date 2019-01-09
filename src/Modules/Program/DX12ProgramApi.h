#pragma once

#include <Context/DX12Context.h>
#include "Program/ProgramApi.h"
#include <Shader/ShaderBase.h>
#include <algorithm>
#include <deque>
#include <array>
#include <set>
#include <unordered_map>

#include "Context/DescriptorPool.h"

#include "CommonProgramApi.h"
#include "IShaderBlobProvider.h"
#include "DX12ViewCreater.h"

class DX12ProgramApi
    : public CommonProgramApi
{
public:
    DX12ProgramApi(DX12Context& context);

    virtual void LinkProgram() override;
    virtual void UseProgram() override;
    virtual void ApplyBindings() override;
    virtual void CompileShader(const ShaderBase& shader) override;
    virtual void ClearRenderTarget(uint32_t slot, const FLOAT ColorRGBA[4]) override;
    virtual void ClearDepthStencil(UINT ClearFlags, FLOAT Depth, UINT8 Stencil) override;
    virtual void SetRasterizeState(const RasterizerDesc& desc) override;
    virtual void SetBlendState(const BlendDesc& desc) override;
    virtual void SetDepthStencilState(const DepthStencilDesc& desc) override;

    virtual ShaderBlob GetBlobByType(ShaderType type) const override;

    void OnPresent();

private:
    DX12Resource::Ptr CreateCBuffer(size_t buffer_size);

    virtual void OnAttachSRV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires) override;
    virtual void OnAttachUAV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires) override;
    virtual void OnAttachCBV(ShaderType type, uint32_t slot, const Resource::Ptr& ires) override;
    virtual void OnAttachSampler(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires) override;
    virtual void OnAttachRTV(uint32_t slot, const Resource::Ptr& ires) override;
    virtual void OnAttachDSV(const Resource::Ptr& ires) override;

    void SetRootSignature(ID3D12RootSignature* pRootSignature);
    void SetRootDescriptorTable(UINT RootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor);
    void SetRootConstantBufferView(UINT RootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);
    std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputLayout(ComPtr<ID3D12ShaderReflection> reflector);
    void CreateGraphicsPSO();
    void CreateComputePSO();
    void CopyDescriptor(DescriptorHeapRange & dst_range, size_t dst_offset, const View::Ptr& view);
    DX12View* ConvertView(const View::Ptr& view);
    View::Ptr CreateView(const BindKey& bind_key, const ViewDesc& view_desc, const Resource::Ptr& res) override;
    
    void ParseShaders();
    void OMSetRenderTargets();

private:
    decltype(&::D3DReflect) _D3DReflect = &::D3DReflect;

    uint32_t m_num_cbv = 0;
    uint32_t m_num_srv = 0;
    uint32_t m_num_uav = 0;
    uint32_t m_num_rtv = 0;
    uint32_t m_num_sampler = 0;

    std::map<ShaderType, ComPtr<ID3DBlob>> m_blob_map;
    bool m_is_compute = false;

    D3D12_ROOT_DESCRIPTOR_TABLE DescriptorTable;
    D3D12_ROOT_CONSTANTS Constants;
    D3D12_ROOT_DESCRIPTOR Descriptor;

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
            } table;

            struct
            {
                size_t root_param_num;
            } view;
        };
    };

    std::map<std::tuple<ShaderType, D3D12_DESCRIPTOR_RANGE_TYPE>, BindingLayout> m_binding_layout;
    ComPtr<ID3D12RootSignature> m_root_signature;

    class DiffAccumulator
    {
    public:
        template<typename T>
        class AssignmentProxy
        {
        public:
            AssignmentProxy(T& dst, bool& has_changed)
                : m_dst(dst)
                , m_has_changed(has_changed)
            {
            }

            template<typename U>
            void operator=(const U& src)
            {
                m_has_changed |= m_dst == static_cast<T>(src);
                m_dst = src;
            }
        private:
            T& m_dst;
            bool& m_has_changed;
        };
 
        template<typename T>
        DiffAccumulator::AssignmentProxy<T> operator()(T& dst)
        {
            return AssignmentProxy<T>(dst, m_has_changed);
        }

        DiffAccumulator& operator=(const bool& val)
        {
            m_has_changed = val;
            return *this;
        }

        operator bool()
        {
            return m_has_changed;
        }

    private:
        bool m_has_changed = false;
    };

    DiffAccumulator m_pso_desc_cache;
     
    D3D12_GRAPHICS_PIPELINE_STATE_DESC m_pso_desc = {};
    D3D12_COMPUTE_PIPELINE_STATE_DESC m_compute_pso_desc = {};

    template<typename T>
    struct Hasher
    {
        std::size_t operator()(const T& oid) const
        {
            const uint8_t* data = reinterpret_cast<const uint8_t*>(&oid);
            auto size = sizeof(T);
            std::size_t prime = 31;
            std::size_t p_pow = 1;
            std::size_t hash = 0;
            for (size_t i = 0; i < size; ++i)
            {
                hash += (*data + 1ll) * p_pow;
                p_pow *= prime;
                ++data;
            }
            return hash;
        }
    };

    template<typename T>
    class EqualFn
    {
    public:
        bool operator() (const T& t1, const T& t2) const
        {
            return memcmp(&t1, &t2, sizeof(T)) == 0;
        }
    };

    std::unordered_map<D3D12_GRAPHICS_PIPELINE_STATE_DESC, ComPtr<ID3D12PipelineState>,
        Hasher<D3D12_GRAPHICS_PIPELINE_STATE_DESC>, 
        EqualFn<D3D12_GRAPHICS_PIPELINE_STATE_DESC>> m_pso;
    ComPtr<ID3D12PipelineState> m_current_pso;

    std::vector<D3D12_INPUT_ELEMENT_DESC> m_input_layout;
    ComPtr<ID3D12ShaderReflection> m_input_layout_reflector;

    const bool m_use_cbv_table = true;
    bool m_changed_binding = false;
    bool m_changed_om = false;
    DX12Context& m_context;
    DX12ViewCreater m_view_creater;
    std::map<std::map<BindKey, View::Ptr>, DescriptorHeapRange> m_heap_cache;
};
