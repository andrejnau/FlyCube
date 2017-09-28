#pragma once

#include "Program/IShaderBuffer.h"
#include "Program/ShaderType.h"
#include <d3d11.h>
#include <d3d11shader.h>
#include <wrl.h>
#include <string>
#include <vector>
#include <map>

using namespace Microsoft::WRL;

class DX11ShaderBuffer : public IShaderBuffer
{
public:
    DX11ShaderBuffer(ComPtr<ID3D11Device>& device, ShaderType shader_type, ComPtr<ID3D11ShaderReflection>& reflector, UINT index);
    virtual BufferProxy Uniform(const std::string& name) override;
    void Update(ComPtr<ID3D11DeviceContext>& device_context);
    void SetOnPipeline(ComPtr<ID3D11DeviceContext>& device_context);

private:
    ComPtr<ID3D11Buffer> CreateBuffer(ComPtr<ID3D11Device>& device, UINT buffer_size);

    struct VDesc
    {
        size_t size;
        size_t offset;
    };

    std::map<std::string, VDesc> m_vdesc;
    UINT m_slot;
    ShaderType m_shader_type;
    ComPtr<ID3D11Buffer> m_buffer;
    std::vector<char> m_data;
};
