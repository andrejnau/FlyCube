#pragma once
#include "PipelineProgram/PipelineProgram.h"
#include <Shader/Shader.h>
#include <vector>
#include <d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXDevice;
class DXShader;

class DXPipelineProgram : public PipelineProgram
{
public:
    DXPipelineProgram(DXDevice& device, const std::vector<std::shared_ptr<Shader>>& shaders);

    const std::vector<std::shared_ptr<DXShader>>& GetShaders() const;
    const ComPtr<ID3D12RootSignature>& GetRootSignature() const;

private:
    DXDevice& m_device;
    std::vector<std::shared_ptr<DXShader>> m_shaders;

    const bool m_use_cbv_table = true;
    uint32_t m_num_cbv = 0;
    uint32_t m_num_srv = 0;
    uint32_t m_num_uav = 0;
    uint32_t m_num_rtv = 0;
    uint32_t m_num_sampler = 0;
    ComPtr<ID3D12RootSignature> m_root_signature;

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
};
