#include "DXSample.h"
#include <chrono>
#include <Util.h>
#include <Utility.h>
#include <Utilities/FileUtility.h>
#include <state.h>
#include <GLFW/glfw3.h>

DXSample::DXSample()
    : m_width(0)
    , m_height(0)
{}

DXSample::~DXSample()
{}

void DXSample::OnInit(int width, int height)
{
    m_width = width;
    m_height = height;

    CreateDeviceAndSwapChain();

    m_shader_geometry_pass.reset(new DXShader(m_device, "shaders/DX11/GeometryPass_VS.hlsl", "shaders/DX11/GeometryPass_PS.hlsl"));
    m_shader_light_pass.reset(new DXShader(m_device, "shaders/DX11/LightPass_VS.hlsl", "shaders/DX11/LightPass_PS.hlsl"));

    CreateRT();
    CreateViewPort();
    CreateSampler();
    m_model_of_file = CreateGeometry("model/sponza_png/sponza.obj");
    m_model_square = CreateGeometry("model/square.obj");
    InitGBuffer();
    camera_.SetViewport(m_width, m_height);

    m_device_context->PSSetSamplers(0, 1, m_texture_sampler.GetAddressOf());
    m_device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void DXSample::OnUpdate()
{
    UpdateCameraMovement();

    static std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
    static std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
    static float angle = 0.0;

    end = std::chrono::system_clock::now();
    int64_t elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    start = std::chrono::system_clock::now();

    /*if (m_use_rotare)
        angle += elapsed / 2e6f;*/

    glm::mat4 projection, view, model;
    camera_.GetMatrix(projection, view, model);

    float model_scale_ = 0.01f;
    model = glm::scale(glm::vec3(model_scale_)) * model;

    auto& constant_buffer_geometry_pass = m_shader_geometry_pass->GetVSCBuffer("ConstantBuffer");
    constant_buffer_geometry_pass.Uniform("model") = glm::transpose(model);
    constant_buffer_geometry_pass.Uniform("view") = glm::transpose(view);
    constant_buffer_geometry_pass.Uniform("projection") = glm::transpose(projection);
    constant_buffer_geometry_pass.Uniform("normalMatrix") = glm::inverse(model);
    constant_buffer_geometry_pass.Update(m_device_context);

    float light_r = 2.5;
    glm::vec3 light_pos_ = glm::vec3(light_r * cos(angle), 25.0f, light_r * sin(angle));

    glm::vec3 cameraPosView = glm::vec3(glm::vec4(camera_.GetCameraPos(), 1.0));
    glm::vec3 lightPosView = glm::vec3(glm::vec4(light_pos_, 1.0));

    auto& constant_buffer_light_pass = m_shader_light_pass->GetPSCBuffer("ConstantBuffer");
    constant_buffer_light_pass.Uniform("lightPos") = glm::vec4(lightPosView, 0.0);
    constant_buffer_light_pass.Uniform("viewPos") = glm::vec4(cameraPosView, 0.0);
    constant_buffer_light_pass.Update(m_device_context);
}

void DXSample::OnRender()
{
    GeometryPass();
    LightPass();
    ASSERT_SUCCEEDED(m_swap_chain->Present(0, 0));
}

void DXSample::GeometryPass()
{
    m_device_context->RSSetViewports(1, &m_viewport);
    m_device_context->VSSetShader(m_shader_geometry_pass->vertex_shader.Get(), 0, 0);
    m_device_context->PSSetShader(m_shader_geometry_pass->pixel_shader.Get(), 0, 0);

    m_shader_geometry_pass->GetVSCBuffer("ConstantBuffer").SetOnPipeline(m_device_context);
    m_shader_geometry_pass->GetPSCBuffer("TexturesEnables").SetOnPipeline(m_device_context);
    m_shader_geometry_pass->GetPSCBuffer("Material").SetOnPipeline(m_device_context);
    m_shader_geometry_pass->GetPSCBuffer("Light").SetOnPipeline(m_device_context);
    m_device_context->IASetInputLayout(m_shader_geometry_pass->input_layout.Get());

    std::vector<ID3D11RenderTargetView*> rtvs = {
        m_position_rtv.Get(),
        m_normal_rtv.Get(),
        m_ambient_rtv.Get(),
        m_diffuse_rtv.Get(),
        m_specular_rtv.Get(),
        m_gloss_rtv.Get(),
    };

    float bgColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    for (auto & rtv : rtvs)
    {
        m_device_context->ClearRenderTargetView(rtv, bgColor);
    }
    m_device_context->ClearDepthStencilView(m_depth_stencil_view.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    m_device_context->OMSetRenderTargets(rtvs.size(), rtvs.data(), m_depth_stencil_view.Get());

    for (size_t mesh_id = 0; mesh_id < m_model_of_file->meshes.size(); ++mesh_id)
    {
        DX11Mesh & cur_mesh = m_model_of_file->meshes[mesh_id];

        UINT stride = sizeof(cur_mesh.vertices[0]);
        UINT offset = 0;
        m_device_context->IASetVertexBuffers(0, 1, cur_mesh.vertBuffer.GetAddressOf(), &stride, &offset);
        m_device_context->IASetIndexBuffer(cur_mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

        std::vector<ID3D11ShaderResourceView*> use_textures(5, nullptr);
        auto& textures_enables = m_shader_geometry_pass->GetPSCBuffer("TexturesEnables");

        textures_enables.Uniform("has_diffuseMap") = 0;
        textures_enables.Uniform("has_normalMap") = 0;
        textures_enables.Uniform("has_specularMap") = 0;
        textures_enables.Uniform("has_glossMap") = 0;
        textures_enables.Uniform("has_ambientMap") = 0;
        textures_enables.Uniform("has_alphaMap") = 0;

        static uint32_t frameId = 0;
        for (size_t i = 0; i < cur_mesh.textures.size(); ++i)
        {
            uint32_t texture_slot = 0;
            switch (cur_mesh.textures[i].type)
            {
            case aiTextureType_DIFFUSE:
                texture_slot = 0;
                textures_enables.Uniform("has_diffuseMap") = 1;
                break;
            case aiTextureType_HEIGHT:
            {
                texture_slot = 1;
                auto& state = CurState<bool>::Instance().state;
                if (!state["disable_norm"])
                    textures_enables.Uniform("has_normalMap") = 1;
            } break;
            case aiTextureType_SPECULAR:
                texture_slot = 2;
                textures_enables.Uniform("has_specularMap") = 1;
                break;
            case aiTextureType_SHININESS:
                texture_slot = 3;
                textures_enables.Uniform("has_glossMap") = 1;
                break;
            case aiTextureType_AMBIENT:
                texture_slot = 4;
                textures_enables.Uniform("has_ambientMap") = 1;
                break;
            case aiTextureType_OPACITY:
                texture_slot = 5;
                textures_enables.Uniform("has_alphaMap") = 1;
            default:
                continue;
            }

            use_textures[texture_slot] = cur_mesh.texResources[i].Get();
        }

        textures_enables.Update(m_device_context);

        auto& material = m_shader_geometry_pass->GetPSCBuffer("Material");
        material.Uniform("material_ambient") = cur_mesh.material.amb;
        material.Uniform("material_diffuse") = cur_mesh.material.dif;
        material.Uniform("material_specular") = cur_mesh.material.spec;
        material.Uniform("material_shininess") = cur_mesh.material.shininess;
        material.Update(m_device_context);

        auto& light = m_shader_geometry_pass->GetPSCBuffer("Light");
        light.Uniform("light_ambient") = glm::vec3(0.2f);
        light.Uniform("light_diffuse") = glm::vec3(1.0f);
        light.Uniform("light_specular") = glm::vec3(0.5f);
        light.Update(m_device_context);

        m_device_context->PSSetShaderResources(0, use_textures.size(), use_textures.data());
        m_device_context->DrawIndexed(cur_mesh.indices.size(), 0, 0);
    }
}

void DXSample::LightPass()
{
    m_device_context->VSSetShader(m_shader_light_pass->vertex_shader.Get(), 0, 0);
    m_device_context->PSSetShader(m_shader_light_pass->pixel_shader.Get(), 0, 0);
    m_shader_light_pass->GetPSCBuffer("ConstantBuffer").SetOnPipeline(m_device_context);
    m_device_context->IASetInputLayout(m_shader_light_pass->input_layout.Get());

    m_device_context->OMSetRenderTargets(1, m_render_target_view.GetAddressOf(), m_depth_stencil_view.Get());

    float bgColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_device_context->ClearRenderTargetView(m_render_target_view.Get(), bgColor);
    m_device_context->ClearDepthStencilView(m_depth_stencil_view.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    for (size_t mesh_id = 0; mesh_id < m_model_square->meshes.size(); ++mesh_id)
    {
        DX11Mesh & cur_mesh = m_model_square->meshes[mesh_id];

        UINT stride = sizeof(cur_mesh.vertices[0]);
        UINT offset = 0;
        m_device_context->IASetVertexBuffers(0, 1, cur_mesh.vertBuffer.GetAddressOf(), &stride, &offset);
        m_device_context->IASetIndexBuffer(cur_mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

        std::vector<ID3D11ShaderResourceView*> use_textures =
        {
            m_position_srv.Get(),
            m_normal_srv.Get(),
            m_ambient_srv.Get(),
            m_diffuse_srv.Get(),
            m_specular_srv.Get(),
            m_gloss_srv.Get()
        };
    
        m_device_context->PSSetShaderResources(0, use_textures.size(), use_textures.data());
        m_device_context->DrawIndexed(cur_mesh.indices.size(), 0, 0);
    }
}

void DXSample::OnDestroy()
{
}

void DXSample::OnSizeChanged(int width, int height)
{
    if (width != m_width || height != m_height)
    {
        m_width = width;
        m_height = height;

        m_render_target_view.Reset();

        DXGI_SWAP_CHAIN_DESC desc = {};
        ASSERT_SUCCEEDED(m_swap_chain->GetDesc(&desc));
        ASSERT_SUCCEEDED(m_swap_chain->ResizeBuffers(1, width, height, desc.BufferDesc.Format, desc.Flags));

        CreateRT();
        CreateViewPort();
        InitGBuffer();
        camera_.SetViewport(m_width, m_height);
    }
}

void DXSample::CreateDeviceAndSwapChain()
{    
    //Describe our Buffer
    DXGI_MODE_DESC bufferDesc;
    ZeroMemory(&bufferDesc, sizeof(DXGI_MODE_DESC));
    bufferDesc.Width = m_width;
    bufferDesc.Height = m_height;
    bufferDesc.RefreshRate.Numerator = 60;
    bufferDesc.RefreshRate.Denominator = 1;
    bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

    //Describe our SwapChain
    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
    swapChainDesc.BufferDesc = bufferDesc;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 1;
    swapChainDesc.OutputWindow = GetActiveWindow();
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    DWORD create_device_flags = 0;
#if 0
    create_device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    ASSERT_SUCCEEDED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, create_device_flags, nullptr, 0,
        D3D11_SDK_VERSION, &swapChainDesc, &m_swap_chain, &m_device, nullptr, &m_device_context));
}

void DXSample::CreateRT()
{
    //Create our BackBuffer
    ComPtr<ID3D11Texture2D> BackBuffer;
    ASSERT_SUCCEEDED(m_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&BackBuffer));

    //Create our Render Target
    ASSERT_SUCCEEDED(m_device->CreateRenderTargetView(BackBuffer.Get(), nullptr, &m_render_target_view));

    //Describe our Depth/Stencil Buffer
    D3D11_TEXTURE2D_DESC depthStencilDesc;
    depthStencilDesc.Width = m_width;
    depthStencilDesc.Height = m_height;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.ArraySize = 1;
    depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
    depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthStencilDesc.CPUAccessFlags = 0;
    depthStencilDesc.MiscFlags = 0;

    ASSERT_SUCCEEDED(m_device->CreateTexture2D(&depthStencilDesc, nullptr, &m_depth_stencil_buffer));
    ASSERT_SUCCEEDED(m_device->CreateDepthStencilView(m_depth_stencil_buffer.Get(), nullptr, &m_depth_stencil_view));
}

void DXSample::CreateViewPort()
{
    ZeroMemory(&m_viewport, sizeof(D3D11_VIEWPORT));
    m_viewport.TopLeftX = 0;
    m_viewport.TopLeftY = 0;
    m_viewport.Width = m_width;
    m_viewport.Height = m_height;
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;
}

void DXSample::CreateSampler()
{
    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory(&sampDesc, sizeof(sampDesc));
    sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

    ASSERT_SUCCEEDED(m_device->CreateSamplerState(&sampDesc, &m_texture_sampler));
}

std::unique_ptr<Model<DX11Mesh>> DXSample::CreateGeometry(const std::string& path)
{
    std::unique_ptr<Model<DX11Mesh>> model = std::make_unique<Model<DX11Mesh>>(path);
     
    for (DX11Mesh & cur_mesh : model->meshes)
    {
        cur_mesh.SetupMesh(m_device, m_device_context);
    } 
    return model;
}

void DXSample::CreateRTV(ComPtr<ID3D11RenderTargetView>& rtv, ComPtr<ID3D11ShaderResourceView>& srv)
{
    D3D11_TEXTURE2D_DESC texture_desc;
    ZeroMemory(&texture_desc, sizeof(texture_desc));
    texture_desc.Width = m_width;
    texture_desc.Height = m_height;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.Usage = D3D11_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    ComPtr<ID3D11Texture2D> texture;
    ASSERT_SUCCEEDED(m_device->CreateTexture2D(&texture_desc, nullptr, &texture));

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    
    ASSERT_SUCCEEDED(m_device->CreateShaderResourceView(texture.Get(), &srvDesc, &srv));

    ASSERT_SUCCEEDED(m_device->CreateRenderTargetView(texture.Get(), nullptr, &rtv));
}

void DXSample::InitGBuffer()
{
    CreateRTV(m_position_rtv, m_position_srv);
    CreateRTV(m_normal_rtv, m_normal_srv);
    CreateRTV(m_ambient_rtv, m_ambient_srv);
    CreateRTV(m_diffuse_rtv, m_diffuse_srv);
    CreateRTV(m_specular_rtv, m_specular_srv);
    CreateRTV(m_gloss_rtv, m_gloss_srv);
}

void DXSample::UpdateCameraMovement()
{
    double currentFrame = glfwGetTime();
    delta_time_ = (float)(currentFrame - last_frame_);
    last_frame_ = currentFrame;

    if (keys_[GLFW_KEY_W])
        camera_.ProcessKeyboard(CameraMovement::kForward, delta_time_);
    if (keys_[GLFW_KEY_S])
        camera_.ProcessKeyboard(CameraMovement::kBackward, delta_time_);
    if (keys_[GLFW_KEY_A])
        camera_.ProcessKeyboard(CameraMovement::kLeft, delta_time_);
    if (keys_[GLFW_KEY_D])
        camera_.ProcessKeyboard(CameraMovement::kRight, delta_time_);
    if (keys_[GLFW_KEY_Q])
        camera_.ProcessKeyboard(CameraMovement::kDown, delta_time_);
    if (keys_[GLFW_KEY_E])
        camera_.ProcessKeyboard(CameraMovement::kUp, delta_time_);
}

void DXSample::OnKey(int key, int action)
{
    if (action == GLFW_PRESS)
        keys_[key] = true;
    else if (action == GLFW_RELEASE)
        keys_[key] = false;

    if (key == GLFW_KEY_N && action == GLFW_PRESS)
    {
        auto & state = CurState<bool>::Instance().state;
        state["disable_norm"] = !state["disable_norm"];
    }
}

void DXSample::OnMouse(bool first_event, double xpos, double ypos)
{
    if (first_event)
    {
        last_x_ = xpos;
        last_y_ = ypos;
    }

    double xoffset = xpos - last_x_;
    double yoffset = last_y_ - ypos;  // Reversed since y-coordinates go from bottom to left

    last_x_ = xpos;
    last_y_ = ypos;

    camera_.ProcessMouseMovement((float)xoffset, (float)yoffset);
}
