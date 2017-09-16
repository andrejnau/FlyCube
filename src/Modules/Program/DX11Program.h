#pragma once

#include "DX11ShaderBuffer.h"
#include <d3d11.h>
#include <wrl/client.h>
#include <map>
#include <string>

using namespace Microsoft::WRL;

class DXShader
{
public:
    DXShader(ComPtr<ID3D11Device>& device, const std::string& vertex, const std::string& pixel);
    DX11ShaderBuffer& GetVSCBuffer(const std::string& name);
    DX11ShaderBuffer& GetPSCBuffer(const std::string& name);

    ComPtr<ID3D11VertexShader> vertex_shader;
    ComPtr<ID3D11PixelShader> pixel_shader;
    ComPtr<ID3D11InputLayout> input_layout;

private:
    ComPtr<ID3DBlob> CompileShader(ComPtr<ID3D11Device>& device, const std::string& shader_path, const std::string& entrypoint, const std::string& target);
    void ParseVariable(ComPtr<ID3D11Device>& device, ShaderType shader_type, ComPtr<ID3DBlob>& shader_buffer, std::map<std::string, DX11ShaderBuffer>& buffers);

    ComPtr<ID3DBlob> m_vertex_shader_buffer;
    ComPtr<ID3DBlob> m_pixel_shader_buffer;

    std::map<std::string, DX11ShaderBuffer> m_vs_buffers;
    std::map<std::string, DX11ShaderBuffer> m_ps_buffers;
};
