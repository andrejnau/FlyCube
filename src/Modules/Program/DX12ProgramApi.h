#pragma once

#define NOMINMAX
#include <Context/DX12Context.h>
#include "Program/ProgramApi.h"
#include <algorithm>
#include <deque>
#include <array>

class DX12ProgramApi : public ProgramApi
{
public:
    DX12ProgramApi(DX12Context& context)
        : m_context(context)
    {
    }

    size_t m_num_rtvs = 0;
    size_t m_num_res = 0;
    size_t m_num_samplers = 0;
    std::map<ShaderType, ComPtr<ID3DBlob>> m_blob_map;

    std::map<ShaderType, std::map<D3D12_DESCRIPTOR_RANGE_TYPE, size_t>> m_heap_offset_map;
    std::map<ShaderType, std::map<D3D12_DESCRIPTOR_RANGE_TYPE, size_t>> m_root_param_map;
    std::map<ShaderType, size_t> m_root_param_start_heap_map;
    std::map<ShaderType, size_t> m_root_param_start_heap_map_sampler;

    std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputLayout(ComPtr<ID3D12ShaderReflection> reflector)
    {
        reflector.Get()->AddRef(); // fix me
        D3D12_SHADER_DESC shader_desc = {};
        reflector->GetDesc(&shader_desc);

        std::vector<D3D12_INPUT_ELEMENT_DESC> input_layout_desc;
        for (UINT i = 0; i < shader_desc.InputParameters; ++i)
        {
            D3D12_SIGNATURE_PARAMETER_DESC param_desc = {};
            reflector->GetInputParameterDesc(i, &param_desc);

            D3D12_INPUT_ELEMENT_DESC layout = {};
            layout.SemanticName = param_desc.SemanticName;
            layout.SemanticIndex = param_desc.SemanticIndex;
            layout.InputSlot = i;
            layout.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            layout.InstanceDataStepRate = 0;

            if (param_desc.Mask == 1)
            {
                if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                    layout.Format = DXGI_FORMAT_R32_UINT;
                else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                    layout.Format = DXGI_FORMAT_R32_SINT;
                else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                    layout.Format = DXGI_FORMAT_R32_FLOAT;
            }
            else if (param_desc.Mask <= 3)
            {
                if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                    layout.Format = DXGI_FORMAT_R32G32_UINT;
                else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                    layout.Format = DXGI_FORMAT_R32G32_SINT;
                else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                    layout.Format = DXGI_FORMAT_R32G32_FLOAT;
            }
            else if (param_desc.Mask <= 7)
            {
                if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                    layout.Format = DXGI_FORMAT_R32G32B32_UINT;
                else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                    layout.Format = DXGI_FORMAT_R32G32B32_SINT;
                else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                    layout.Format = DXGI_FORMAT_R32G32B32_FLOAT;
            }
            else if (param_desc.Mask <= 15)
            {
                if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                    layout.Format = DXGI_FORMAT_R32G32B32A32_UINT;
                else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                    layout.Format = DXGI_FORMAT_R32G32B32A32_SINT;
                else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                    layout.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            }
            input_layout_desc.push_back(layout);
        }

        return input_layout_desc;
    }
    void NextDraw()
    {
        ++draw_offset;
    }

    bool is_first_draw;

    void BeforeDraw()
    {
        if (!is_first_draw)
            return;
        is_first_draw = false;
        ComPtr<ID3D12ShaderReflection> reflector;
        D3DReflect(m_blob_map[ShaderType::kVertex]->GetBufferPointer(), m_blob_map[ShaderType::kVertex]->GetBufferSize(), IID_PPV_ARGS(&reflector));

        auto input_layout_elements = GetInputLayout(reflector);
        D3D12_INPUT_LAYOUT_DESC input_layout = {};
        input_layout.NumElements = input_layout_elements.size();
        input_layout.pInputElementDescs = input_layout_elements.data();

        if (psoDesc.RTVFormats[0] != 0)
        {
            int b = 0;
        }

        psoDesc.RasterizerState = {};

        //if (psoDesc.DSVFormat == DXGI_FORMAT_D32_FLOAT)
        {
            psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
            psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
            psoDesc.RasterizerState.DepthBias = 4096;
            psoDesc.RasterizerState.DepthClipEnable = false;
        }

        psoDesc.InputLayout = input_layout; // the structure describing our input layout
        psoDesc.pRootSignature = rootSignature.Get(); // the root signature that describes the input data this pso needs
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // type of topology we are drawing

        DXGI_SAMPLE_DESC sampleDesc = {};
        sampleDesc.Count = 1; // multisample count (no multisampling, so we just put 1, since we still need 1 sample)

        psoDesc.SampleDesc = sampleDesc; // must be the same sample description as the swapchain and depth/stencil buffer
        psoDesc.SampleMask = UINT_MAX; // sample mask has to do with multi-sampling. 0xffffffff means point sampling is done
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); // a default rasterizer state.
        D3D12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = blendDesc; // a default blent state.
        psoDesc.NumRenderTargets = m_num_rtvs; // we are only binding one render target


        CD3DX12_DEPTH_STENCIL_DESC depthStencilDesc(D3D12_DEFAULT);
        depthStencilDesc.DepthEnable = true;
        depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        depthStencilDesc.StencilEnable = FALSE;

        psoDesc.DepthStencilState = depthStencilDesc; // a default depth stencil state
        ASSERT_SUCCEEDED(m_context.device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineStateObject)));

        m_context.commandList->SetPipelineState(pipelineStateObject.Get());
    }

    void UseProgram(size_t draw_calls) override
    {
        is_first_draw = true;

        draw_offset = 0;
        m_context.current_program = this;
        size_t num_resources = 0;
        size_t num_samplers = 0;
        size_t num_rtvs = 0;

        std::vector<D3D12_ROOT_PARAMETER> rootParameters;
        std::deque<std::array<D3D12_DESCRIPTOR_RANGE, 4>> descriptorTableRanges;
        for (auto& shader_blob : m_blob_map)
        {
            m_root_param_start_heap_map[shader_blob.first] = num_resources;
            m_root_param_start_heap_map_sampler[shader_blob.first] = num_samplers;

            size_t num_cbv = 0;
            size_t num_srv = 0;
            size_t num_uav = 0;
            size_t num_sampler = 0;

            ComPtr<ID3D12ShaderReflection> reflector;
            D3DReflect(shader_blob.second->GetBufferPointer(), shader_blob.second->GetBufferSize(), IID_PPV_ARGS(&reflector));

            D3D12_SHADER_DESC desc = {};
            reflector->GetDesc(&desc);
            for (UINT i = 0; i < desc.BoundResources; ++i)
            {
                D3D12_SHADER_INPUT_BIND_DESC res_desc = {};
                ASSERT_SUCCEEDED(reflector->GetResourceBindingDesc(i, &res_desc));
                switch (res_desc.Type)
                {
                case D3D_SIT_SAMPLER:
                    num_sampler = std::max<size_t>(num_sampler, res_desc.BindPoint + res_desc.BindCount);
                    break;
                case D3D_SIT_CBUFFER:
                    num_cbv = std::max<size_t>(num_cbv, res_desc.BindPoint + res_desc.BindCount);
                    break;
                case D3D_SIT_TBUFFER:
                case D3D_SIT_TEXTURE:
                case D3D_SIT_STRUCTURED:
                case D3D_SIT_BYTEADDRESS:
                    num_srv = std::max<size_t>(num_srv, res_desc.BindPoint + res_desc.BindCount);
                    break;
                default:
                    num_uav = std::max<size_t>(num_uav, res_desc.BindPoint + res_desc.BindCount);
                    break;
                }
            }

            if (shader_blob.first == ShaderType::kPixel)
            {
                for (UINT i = 0; i < desc.OutputParameters; ++i)
                {
                    D3D12_SIGNATURE_PARAMETER_DESC param_desc = {};
                    reflector->GetOutputParameterDesc(i, &param_desc);
                    num_rtvs = std::max<size_t>(num_rtvs, param_desc.Register + 1);
                }
            }

            D3D12_SHADER_VISIBILITY visibility;
            switch (shader_blob.first)
            {
            case ShaderType::kVertex:
                visibility = D3D12_SHADER_VISIBILITY_VERTEX;
                break;
            case ShaderType::kPixel:
                visibility = D3D12_SHADER_VISIBILITY_PIXEL;
                break;
            case ShaderType::kCompute:
                visibility = D3D12_SHADER_VISIBILITY_ALL;
                break;
            case ShaderType::kGeometry:
                visibility = D3D12_SHADER_VISIBILITY_GEOMETRY;
                break;
            }

          

            descriptorTableRanges.emplace_back();
            size_t index = 0;

            m_heap_offset_map[shader_blob.first][D3D12_DESCRIPTOR_RANGE_TYPE_SRV] = num_resources;
            m_heap_offset_map[shader_blob.first][D3D12_DESCRIPTOR_RANGE_TYPE_UAV] = num_resources + num_srv;
            m_heap_offset_map[shader_blob.first][D3D12_DESCRIPTOR_RANGE_TYPE_CBV] = num_resources + num_srv + num_uav;
            m_heap_offset_map[shader_blob.first][D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER] = num_samplers;

            if ((num_srv + num_uav + num_cbv) > 0)
            {
                if (num_srv)
                {
                    descriptorTableRanges.back()[index].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                    descriptorTableRanges.back()[index].NumDescriptors = num_srv;
                    descriptorTableRanges.back()[index].BaseShaderRegister = 0;
                    descriptorTableRanges.back()[index].RegisterSpace = 0;
                    descriptorTableRanges.back()[index].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
                    ++index;
                }

                if (num_uav)
                {
                    descriptorTableRanges.back()[index].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                    descriptorTableRanges.back()[index].NumDescriptors = num_uav;
                    descriptorTableRanges.back()[index].BaseShaderRegister = 0;
                    descriptorTableRanges.back()[index].RegisterSpace = 0;
                    descriptorTableRanges.back()[index].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
                    ++index;
                }

                if (num_cbv)
                {
                    descriptorTableRanges.back()[index].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
                    descriptorTableRanges.back()[index].NumDescriptors = num_cbv;
                    descriptorTableRanges.back()[index].BaseShaderRegister = 0;
                    descriptorTableRanges.back()[index].RegisterSpace = 0;
                    descriptorTableRanges.back()[index].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
                    ++index;
                }

                D3D12_ROOT_DESCRIPTOR_TABLE descriptorTableTexture;
                descriptorTableTexture.NumDescriptorRanges = index;
                descriptorTableTexture.pDescriptorRanges = &descriptorTableRanges.back()[0];

                m_root_param_map[shader_blob.first][D3D12_DESCRIPTOR_RANGE_TYPE_SRV] = rootParameters.size();
                m_root_param_map[shader_blob.first][D3D12_DESCRIPTOR_RANGE_TYPE_UAV] = rootParameters.size();
                m_root_param_map[shader_blob.first][D3D12_DESCRIPTOR_RANGE_TYPE_CBV] = rootParameters.size();

                rootParameters.emplace_back();
                rootParameters.back().ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                rootParameters.back().DescriptorTable = descriptorTableTexture;
                rootParameters.back().ShaderVisibility = visibility;
            }
            else
            {
                m_root_param_map[shader_blob.first][D3D12_DESCRIPTOR_RANGE_TYPE_SRV] = -1;
                m_root_param_map[shader_blob.first][D3D12_DESCRIPTOR_RANGE_TYPE_UAV] = -1;
                m_root_param_map[shader_blob.first][D3D12_DESCRIPTOR_RANGE_TYPE_CBV] = -1;
            }

            if (num_sampler)
            {
                descriptorTableRanges.back()[index].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
                descriptorTableRanges.back()[index].NumDescriptors = num_sampler;
                descriptorTableRanges.back()[index].BaseShaderRegister = 0;
                descriptorTableRanges.back()[index].RegisterSpace = 0;
                descriptorTableRanges.back()[index].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

                D3D12_ROOT_DESCRIPTOR_TABLE descriptorTableSampler;
                descriptorTableSampler.NumDescriptorRanges = 1;
                descriptorTableSampler.pDescriptorRanges = &descriptorTableRanges.back()[index];

                m_root_param_map[shader_blob.first][D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER] = rootParameters.size();

                rootParameters.emplace_back();
                rootParameters.back().ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                rootParameters.back().DescriptorTable = descriptorTableSampler;
                rootParameters.back().ShaderVisibility = visibility;
            }
            else
            {
                m_root_param_map[shader_blob.first][D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER] = -1;
            }

            num_resources += num_cbv + num_srv + num_uav;
            num_samplers += num_sampler;
        }

        {
            D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
            heap_desc.NumDescriptors = num_resources * draw_calls;
            heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            m_num_res = num_resources;
            ASSERT_SUCCEEDED(m_context.device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&resource_heap)));
        }

        {
            D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
            heap_desc.NumDescriptors = num_samplers * draw_calls;
            heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
            m_num_samplers = num_samplers;
            ASSERT_SUCCEEDED(m_context.device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&sampler_heap)));
        }

        {
            D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
            heap_desc.NumDescriptors = num_rtvs;
            heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            m_num_rtvs = num_rtvs;
            ASSERT_SUCCEEDED(m_context.device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&rtv_heap)));
        }

        {
            D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
            heap_desc.NumDescriptors = 1;
            heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
            ASSERT_SUCCEEDED(m_context.device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&dsv_heap)));
        }

        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init(rootParameters.size(),
            rootParameters.data(),
            0,
            nullptr,
            rootSignatureFlags);

        ID3DBlob* signature;
        ID3DBlob* errorBuff; // a buffer holding the error data if any
        ASSERT_SUCCEEDED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &errorBuff),
            "%s", (char*)errorBuff->GetBufferPointer());
        ASSERT_SUCCEEDED(m_context.device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));

        m_context.commandList->SetGraphicsRootSignature(rootSignature.Get());
        m_context.commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        ID3D12DescriptorHeap *descriptorHeaps[] = { resource_heap.Get(), sampler_heap.Get() };
        m_context.commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    }

    ComPtr<ID3D12DescriptorHeap> resource_heap;
    ComPtr<ID3D12DescriptorHeap> sampler_heap;
    ComPtr<ID3D12DescriptorHeap> rtv_heap;
    ComPtr<ID3D12DescriptorHeap> dsv_heap;
    ComPtr<ID3D12RootSignature> rootSignature;

    virtual void UpdateData(ComPtr<IUnknown> ires, const void* ptr) override
    {
        m_context.UpdateSubresource(ires, 0, ptr, 0, 0);
    }

    virtual void OnCompileShader(ShaderType type, const ComPtr<ID3DBlob>& blob) override
    {
        m_blob_map[type] = blob;
        D3D12_SHADER_BYTECODE ShaderBytecode = {};
        ShaderBytecode.BytecodeLength = blob->GetBufferSize();
        ShaderBytecode.pShaderBytecode = blob->GetBufferPointer();
        switch (type)
        {
        case ShaderType::kVertex:
            psoDesc.VS = ShaderBytecode;
            break;
        case ShaderType::kPixel:
            psoDesc.PS = ShaderBytecode;
            break;
        case ShaderType::kCompute:
            compute_pso.CS = ShaderBytecode;
            break;
        case ShaderType::kGeometry:
            psoDesc.GS = ShaderBytecode;
            break;
        }
    }

    size_t draw_offset = 0;

    virtual void AttachSRV(ShaderType type, const std::string& name, uint32_t slot, const ComPtr<IUnknown>& res) override
    {
        CreateSrv(type, name, slot, res);

        const UINT cbvSrvDescriptorSize = m_context.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        CD3DX12_GPU_DESCRIPTOR_HANDLE gpucbvSrvHandle(resource_heap->GetGPUDescriptorHandleForHeapStart(),
            draw_offset * m_num_res +  m_root_param_start_heap_map[type], cbvSrvDescriptorSize);

        m_context.commandList->SetGraphicsRootDescriptorTable(m_root_param_map[type][D3D12_DESCRIPTOR_RANGE_TYPE_SRV], gpucbvSrvHandle);
    }

    virtual void AttachUAV(ShaderType type, const std::string& name, uint32_t slot, const ComPtr<IUnknown>& res) override
    {
        CreateUAV(type, name, slot, res);

        const UINT cbvSrvDescriptorSize = m_context.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        CD3DX12_GPU_DESCRIPTOR_HANDLE gpucbvSrvHandle(resource_heap->GetGPUDescriptorHandleForHeapStart(),
            draw_offset * m_num_res +  m_root_param_start_heap_map[type], cbvSrvDescriptorSize);

        m_context.commandList->SetGraphicsRootDescriptorTable(m_root_param_map[type][D3D12_DESCRIPTOR_RANGE_TYPE_UAV], gpucbvSrvHandle);
    }

    virtual void UpdateOmSet() override
    {
        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvss;
        for (int i = 0; i < m_num_rtvs; ++i)
        {
            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtv_heap->GetCPUDescriptorHandleForHeapStart(), i,
                 m_context.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
            const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
            m_context.commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
            rtvss.push_back(rtvHandle);
        }
        D3D12_CPU_DESCRIPTOR_HANDLE om_rtv = rtv_heap->GetCPUDescriptorHandleForHeapStart();
        D3D12_CPU_DESCRIPTOR_HANDLE om_dsv = dsv_heap->GetCPUDescriptorHandleForHeapStart();
        m_context.commandList->OMSetRenderTargets(rtvss.size(), rtvss.data(), FALSE, &om_dsv);


        
        // clear the depth/stencil buffer
        m_context.commandList->ClearDepthStencilView(om_dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

        ComPtr<ID3D12ShaderReflection> reflector;
        D3DReflect(m_blob_map[ShaderType::kVertex]->GetBufferPointer(), m_blob_map[ShaderType::kVertex]->GetBufferSize(), IID_PPV_ARGS(&reflector));

        auto input_layout_elements = GetInputLayout(reflector);
        D3D12_INPUT_LAYOUT_DESC input_layout = {};
        input_layout.NumElements = input_layout_elements.size();
        input_layout.pInputElementDescs = input_layout_elements.data();
        psoDesc.InputLayout = input_layout;

      //  ComPtr<ID3D12PipelineState> tmppipelineStateObject;
      //  ASSERT_SUCCEEDED(m_context.device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&tmppipelineStateObject)));

       // m_context.commandList->SetPipelineState(tmppipelineStateObject.Get());
        //pipelineStateObject = tmppipelineStateObject;
    }

    virtual void AttachRTV(uint32_t slot, const ComPtr<IUnknown>& res) override
    {
        ComPtr<ID3D12Resource> renderTarget;
        res.As(&renderTarget);

        m_context.ResourceBarrier(renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET);

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtv_heap->GetCPUDescriptorHandleForHeapStart(), slot,
            m_context.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));

        psoDesc.RTVFormats[slot] = renderTarget->GetDesc().Format;

        m_context.device->CreateRenderTargetView(renderTarget.Get(), nullptr, rtvHandle);
    }

    virtual void AttachDSV(const ComPtr<IUnknown>& res) override
    {
        if (!res)
            return;
        ComPtr<ID3D12Resource> renderTarget;
        res.As(&renderTarget);

        m_context.ResourceBarrier(renderTarget, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(dsv_heap->GetCPUDescriptorHandleForHeapStart(), 0,
            m_context.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV));

        auto desc = renderTarget->GetDesc();

        psoDesc.DSVFormat = renderTarget->GetDesc().Format;


        if (desc.Format == DXGI_FORMAT_R32_TYPELESS) // TODO
        {
            psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

            D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
            dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
            dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
            dsv_desc.Texture2DArray.ArraySize = desc.DepthOrArraySize;
            m_context.device->CreateDepthStencilView(renderTarget.Get(), &dsv_desc, rtvHandle);

        }
        else
        {
            m_context.device->CreateDepthStencilView(renderTarget.Get(), nullptr, rtvHandle);
        }
    }

    virtual void AttachSampler(ShaderType type, uint32_t slot, const SamplerDesc& desc) override
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle(sampler_heap->GetCPUDescriptorHandleForHeapStart(),
            draw_offset  * m_num_samplers+  m_heap_offset_map[type][D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER] + slot,
            m_context.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER));

        D3D12_SAMPLER_DESC sampler_desc = {};

        switch (desc.filter)
        {
        case SamplerFilter::kAnisotropic:
            sampler_desc.Filter = D3D12_FILTER_ANISOTROPIC;
            break;
        case SamplerFilter::kComparisonMinMagMipLinear:
            sampler_desc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
            break;
        }

        switch (desc.mode)
        {
        case SamplerTextureAddressMode::kWrap:
            sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            break;
        case SamplerTextureAddressMode::kClamp:
            sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            break;
        }

        switch (desc.func)
        {
        case SamplerComparisonFunc::kNever:
            sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
            break;
        case SamplerComparisonFunc::kLess:
            sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS;
            break;
        }

        sampler_desc.MinLOD = 0;
        sampler_desc.MaxLOD = D3D12_FLOAT32_MAX;
        sampler_desc.MaxAnisotropy = 1;

        m_context.device->CreateSampler(&sampler_desc, cbvSrvHandle);

        const UINT cbvSrvDescriptorSize = m_context.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
        CD3DX12_GPU_DESCRIPTOR_HANDLE gpucbvSrvHandle(sampler_heap->GetGPUDescriptorHandleForHeapStart(),
            draw_offset* m_num_samplers +  m_root_param_start_heap_map_sampler[type], cbvSrvDescriptorSize);

        m_context.commandList->SetGraphicsRootDescriptorTable(m_root_param_map[type][D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER], gpucbvSrvHandle);
    }

    virtual void AttachCBuffer(ShaderType type, uint32_t slot, const ComPtr<IUnknown>& res) override
    {

        ComPtr<ID3D12Resource> buf;
        res.As(&buf);

        m_context.ResourceBarrier(buf, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);


        // Describe and create a constant buffer view.
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = buf->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = (buf->GetDesc().Width + 255) & ~255;	// CB size is required to be 256-byte aligned.

        CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle(resource_heap->GetCPUDescriptorHandleForHeapStart(),
            draw_offset * m_num_res +  m_heap_offset_map[type][D3D12_DESCRIPTOR_RANGE_TYPE_CBV] + slot,
            m_context.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

        m_context.device->CreateConstantBufferView(&cbvDesc, cbvSrvHandle);

        const UINT cbvSrvDescriptorSize = m_context.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        CD3DX12_GPU_DESCRIPTOR_HANDLE gpucbvSrvHandle(resource_heap->GetGPUDescriptorHandleForHeapStart(), 
            draw_offset * m_num_res +  m_root_param_start_heap_map[type], cbvSrvDescriptorSize);
        m_context.commandList->SetGraphicsRootDescriptorTable(m_root_param_map[type][D3D12_DESCRIPTOR_RANGE_TYPE_CBV], gpucbvSrvHandle);
    }

    virtual Context& GetContext() override
    {
        return m_context;
    }

private:

    void CreateSrv(ShaderType type, const std::string& name, uint32_t slot, const ComPtr<IUnknown>& ires)
    {
        if (!ires)
            return;

        ComPtr<ID3D12Resource> res;
        ires.As(&res);

        if (type == ShaderType::kPixel)
         m_context.ResourceBarrier(res, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        else
            m_context.ResourceBarrier(res, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        if (!res)
            return;

        ComPtr<ID3D12ShaderReflection> reflector;
        D3DReflect(m_blob_map[type]->GetBufferPointer(), m_blob_map[type]->GetBufferSize(), IID_PPV_ARGS(&reflector));

        D3D12_SHADER_INPUT_BIND_DESC res_desc = {};
        ASSERT_SUCCEEDED(reflector->GetResourceBindingDescByName(name.c_str(), &res_desc));

        D3D12_RESOURCE_DESC desc = res->GetDesc();

        CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle(resource_heap->GetCPUDescriptorHandleForHeapStart(),
            draw_offset * m_num_res + m_heap_offset_map[type][D3D12_DESCRIPTOR_RANGE_TYPE_SRV] + slot,
            m_context.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

        switch (res_desc.Dimension)
        {
        case D3D_SRV_DIMENSION_BUFFER:
        {
            size_t stride = 0;
            UINT stride_size = sizeof(stride);
            auto hr = res->GetPrivateData(buffer_stride, &stride_size, &stride);

            D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
            srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srv_desc.Format = DXGI_FORMAT_UNKNOWN;
            srv_desc.Buffer.FirstElement = 0;
            srv_desc.Buffer.NumElements = desc.Width / stride;
            srv_desc.Buffer.StructureByteStride = stride;
            srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            m_context.device->CreateShaderResourceView(res.Get(), &srv_desc, cbvSrvHandle);

            break;
        }
        case D3D_SRV_DIMENSION_TEXTURE2D:
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
            srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srv_desc.Format = desc.Format;
            srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srv_desc.Texture2D.MipLevels = desc.MipLevels;
            m_context.device->CreateShaderResourceView(res.Get(), &srv_desc, cbvSrvHandle);
            break;
        }
        case D3D_SRV_DIMENSION_TEXTURE2DMS:
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
            srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srv_desc.Format = desc.Format;
            srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
            m_context.device->CreateShaderResourceView(res.Get(), &srv_desc, cbvSrvHandle);
            break;
        }
        case D3D_SRV_DIMENSION_TEXTURECUBE:
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
            srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srv_desc.Format = DXGI_FORMAT_R32_FLOAT; // TODO tex_dec.Format;
            srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
            srv_desc.TextureCube.MipLevels = desc.MipLevels;
            m_context.device->CreateShaderResourceView(res.Get(), &srv_desc, cbvSrvHandle);
            break;
        }
        default:
            assert(false);
            break;
        }
    }

    void CreateUAV(ShaderType type, const std::string& name, uint32_t slot, const ComPtr<IUnknown>& ires)
    {
        ComPtr<ID3D11UnorderedAccessView> uav;
        if (!ires)
            return;

        ComPtr<ID3D12Resource> res;
        ires.As(&res);

        if (!res)
            return;

        ComPtr<ID3D12ShaderReflection> reflector;
        D3DReflect(m_blob_map[type]->GetBufferPointer(), m_blob_map[type]->GetBufferSize(), IID_PPV_ARGS(&reflector));

        D3D12_SHADER_INPUT_BIND_DESC res_desc = {};
        ASSERT_SUCCEEDED(reflector->GetResourceBindingDescByName(name.c_str(), &res_desc));

        D3D12_RESOURCE_DESC desc = res->GetDesc();

        CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle(resource_heap->GetCPUDescriptorHandleForHeapStart(),
            draw_offset * m_num_res + m_heap_offset_map[type][D3D12_DESCRIPTOR_RANGE_TYPE_UAV] + slot,
            m_context.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

        switch (res_desc.Dimension)
        {
        case D3D_SRV_DIMENSION_BUFFER:
        {
            size_t stride = 0;
            UINT stride_size = sizeof(stride);
            auto hr = res->GetPrivateData(buffer_stride, &stride_size, &stride);

            D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
            uav_desc.Format = DXGI_FORMAT_UNKNOWN;
            uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            uav_desc.Buffer.FirstElement = 0;
            uav_desc.Buffer.NumElements = desc.Width / stride;
            uav_desc.Buffer.StructureByteStride = stride;
            m_context.device->CreateUnorderedAccessView(res.Get(), nullptr, &uav_desc, cbvSrvHandle);
     
            break;
        }
        default:
            assert(false);
            break;
        }
        return;
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    D3D12_COMPUTE_PIPELINE_STATE_DESC compute_pso = {};
    ComPtr<ID3D12PipelineState> pipelineStateObject; // pso containing a pipeline state

    DX12Context& m_context;
};
