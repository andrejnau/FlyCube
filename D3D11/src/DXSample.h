#pragma once

#include <d3d11.h>
#include <DXGI1_4.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include <memory>
#include <string>

#include <IDXSample.h>
#include <Win32Application.h>
#include <Util.h>
#include <Geometry.h>
#include "DX11Geometry.h"

using namespace Microsoft::WRL;
using namespace DirectX;

class DXSample : public IDXSample
{
public:
    DXSample(int width, int height);
    ~DXSample();

    virtual void OnInit() override;
    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnDestroy() override;
    virtual void OnSizeChanged(int width, int height) override;

    UINT GetWidth() const  override;
    UINT GetHeight() const  override;

private:
    void CreateDeviceAndSwapChain();
    void CreateRT();
    void CreateViewPort();
    void CreateShaders();
    void CreateLayout();
    void CreateCB();
    void CreateSampler();
    void CreateGeometry();

    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11DeviceContext> m_device_context;
    ComPtr<IDXGISwapChain> m_swap_chain;

    ComPtr<ID3D11RenderTargetView> m_render_target_view;
    ComPtr<ID3D11DepthStencilView> m_depth_stencil_view;
    ComPtr<ID3D11Texture2D> m_depth_stencil_buffer;

    D3D11_VIEWPORT m_viewport;

    ComPtr<ID3D11VertexShader> m_vertex_shader;
    ComPtr<ID3D11PixelShader> m_pixel_shader;
    ComPtr<ID3DBlob> m_vertex_shader_buffer;
    ComPtr<ID3DBlob> m_pixel_shader_buffer;

    ComPtr<ID3D11InputLayout> m_input_layout;

    struct UniformBuffer
    {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 projection;
        glm::vec4 lightPos;
        glm::vec4 viewPos;
    };
    UniformBuffer m_uniform;
    ComPtr<ID3D11Buffer> m_uniform_buffer;

    ComPtr<ID3D11SamplerState> m_texture_sampler;

    struct TexInfo
    {
        std::vector<uint8_t> image;
        int width;
        int height;
        int num_bits_per_pixel;
        int image_size;
        int bytes_per_row;
    };
    DXSample::TexInfo LoadImageDataFromFile(const std::string& filename);

    std::unique_ptr<Model<DX11Mesh>> m_model_of_file;

    int m_width;
    int m_height;
    const bool m_use_rotare = true;
};