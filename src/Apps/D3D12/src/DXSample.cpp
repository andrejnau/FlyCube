#include "DXSample.h"
#include <chrono>
#include <SOIL.h>
#include <Utilities/FileUtility.h>
#include <Utilities/DXUtility.h>
#include <Texture/DXGIFormatHelper.h>

DXSample::DXSample()
    : m_width(0)
    , m_height(0)
{}

DXSample::~DXSample()
{}

void DXSample::CreateDevice()
{
#if defined(_DEBUG)
    // Enable the D3D12 debug layer.
{
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
    }
}
#endif

    ASSERT_SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)));

    auto GetHardwareAdapter = [&](ComPtr<IDXGIFactory4> dxgiFactory) -> ComPtr<IDXGIAdapter1>
    {
        ComPtr<IDXGIAdapter1> adapter; // adapters are the graphics card (this includes the embedded graphics on the motherboard)
        for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != dxgiFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                continue;
            }

            // Check to see if the adapter supports Direct3D 12, but don't create the
            // actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_1, _uuidof(ID3D12Device), nullptr)))
                return adapter;
        }
        return nullptr;
    };

    ComPtr<IDXGIAdapter1> adapter = GetHardwareAdapter(dxgiFactory.Get());
    ASSERT(adapter != nullptr);
    ASSERT_SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS(&device)));
}

void DXSample::CreateCommandQueue()
{
    D3D12_COMMAND_QUEUE_DESC cqDesc = {}; // we will be using all the default values
    ASSERT_SUCCEEDED(device->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&commandQueue))); // create the command queue
}

void DXSample::CreateSwapChain()
{
    DXGI_MODE_DESC backBufferDesc = {}; // this is to describe our display mode
    backBufferDesc.Width = 0; // buffer width
    backBufferDesc.Height = 0; // buffer height
    backBufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // format of the buffer (rgba 32 bits, 8 bits for each chanel)

    // describe our multi-sampling. We are not multi-sampling, so we set the count to 1 (we need at least one sample of course)
    DXGI_SAMPLE_DESC sampleDesc = {};
    sampleDesc.Count = 1; // multisample count (no multisampling, so we just put 1, since we still need 1 sample)

    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = frameBufferCount; // number of buffers we have
    swapChainDesc.BufferDesc = backBufferDesc; // our back buffer description
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // this says the pipeline will render to this swap chain
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // dxgi will discard the buffer (data) after we call present
    swapChainDesc.OutputWindow = GetActiveWindow(); // handle to our window
    swapChainDesc.SampleDesc = sampleDesc; // our multi-sampling description
    swapChainDesc.Windowed = true; // set to true, then if in fullscreen must call SetFullScreenState with true for full screen to get uncapped fps
    ComPtr<IDXGISwapChain> tempSwapChain;
    ASSERT_SUCCEEDED(dxgiFactory->CreateSwapChain(
        commandQueue.Get(), // the queue will be flushed once the swap chain is created
        &swapChainDesc, // give it the swap chain description we created above
        &tempSwapChain // store the created swap chain in a temp IDXGISwapChain interface
        ));
    swapChain = static_cast<IDXGISwapChain3*>(tempSwapChain.Get());
    frameIndex = swapChain->GetCurrentBackBufferIndex();
}

void DXSample::CreateFence()
{
    // create the fences
    for (int i = 0; i < frameBufferCount; ++i)
    {
        ASSERT_SUCCEEDED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence[i])));
        fenceValue[i] = 0; // set the initial fence value to 0
    }

    // create a handle to a fence event
    fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    ASSERT(fenceEvent != nullptr);
}

void DXSample::CreateRT()
{
    // describe an rtv descriptor heap and create
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = frameBufferCount; // number of descriptors for this heap.
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // this heap is a render target view heap
    // This heap will not be directly referenced by the shaders (not shader visible), as this will store the output from the pipeline
    // otherwise we would set the heap's flag to D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ASSERT_SUCCEEDED(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap)));

    // get the size of a descriptor in this heap (this is a rtv heap, so only rtv descriptors should be stored in it.
    // descriptor sizes may vary from device to device, which is why there is no set size and we must ask the
    // device to give us the size. we will use this size to increment a descriptor handle offset
    rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // get a handle to the first descriptor in the descriptor heap. a handle is basically a pointer,
    // but we cannot literally use it like a c++ pointer.
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    // Create a RTV for each buffer (double buffering is two buffers, tripple buffering is 3).
    for (int i = 0; i < frameBufferCount; ++i)
    {
        // first we get the n'th buffer in the swap chain and store it in the n'th
        // position of our ID3D12Resource array
        ASSERT_SUCCEEDED(swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i])));

        // the we "create" a render target view which binds the swap chain buffer (ID3D12Resource[n]) to the rtv handle
        device->CreateRenderTargetView(renderTargets[i].Get(), nullptr, rtvHandle);

        // we increment the rtv handle by the rtv descriptor size we got above
        rtvHandle.Offset(1, rtvDescriptorSize);
    }
}

void DXSample::CreateDepthStencil()
{
    // create a depth stencil descriptor heap so we can get a pointer to the depth stencil buffer
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ASSERT_SUCCEEDED(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsDescriptorHeap)));

    D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
    depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

    D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
    depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
    depthOptimizedClearValue.DepthStencil.Stencil = 0;

    device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, m_width, m_height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &depthOptimizedClearValue,
        IID_PPV_ARGS(&depthStencilBuffer)
        );
    depthStencilBuffer->SetName(L"Depth/Stencil Resource Heap");

    device->CreateDepthStencilView(depthStencilBuffer.Get(), &depthStencilDesc, dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}

void DXSample::CreateCommandAllocators()
{
    for (int i = 0; i < frameBufferCount; ++i)
    {
        ASSERT_SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator[i])));
    }
}

void DXSample::CreateCommandList()
{
    // create the command list with the first allocator
    ASSERT_SUCCEEDED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator[frameIndex].Get(), nullptr, IID_PPV_ARGS(&commandList)));
}

void DXSample::CreateGeometry()
{
    m_modelOfFile.reset(new Model<DX12Mesh>("model/export3dcoat/export3dcoat.obj"));

    uint32_t num_textures = 0;
    for (DX12Mesh & cur_mesh : m_modelOfFile->meshes)
    {
        cur_mesh.SetupMesh(CommandHelper(device, commandList));
        num_textures += (uint32_t)cur_mesh.textures.size();
    }
    // create the descriptor heap that will store our srv
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = num_textures;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    ASSERT_SUCCEEDED(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mainDescriptorTextureHeap)));

    for (size_t i = 0; i < m_modelOfFile->meshes.size(); ++i)
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = max_texture_slot;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        ASSERT_SUCCEEDED(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_modelOfFile->meshes[i].currentDescriptorTextureHeap)));
    }

    uint32_t id = 0;
    for (DX12Mesh & cur_mesh : m_modelOfFile->meshes)
    {
        for (size_t i = 0; i < cur_mesh.textures.size(); ++i)
        {
            cur_mesh.texResources[i].offset = id++;
            CreateTexture(cur_mesh.textures[i], cur_mesh.texResources[i]);
        }
    }
}

void DXSample::CreateConstantBuffer()
{
    // create the constant buffer resource heap
    // We will update the constant buffer one or more times per frame, so we will use only an upload heap
    // unlike previously we used an upload heap to upload the vertex and index data, and then copied over
    // to a default heap. If you plan to use a resource for more than a couple frames, it is usually more
    // efficient to copy to a default heap where it stays on the gpu. In this case, our constant buffer
    // will be modified and uploaded at least once per frame, so we only use an upload heap

    // first we will create a resource heap (upload heap) for each frame for the cubes constant buffers
    // As you can see, we are allocating 64KB for each resource we create. Buffer resource heaps must be
    // an alignment of 64KB. We are creating 3 resources, one for each frame. Each constant buffer is
    // only a 4x4 matrix of floats in this tutorial. So with a float being 4 bytes, we have
    // 16 floats in one constant buffer, and we will store 2 constant buffers in each
    // heap, one for each cube, thats only 64x2 bits, or 128 bits we are using for each
    // resource, and each resource must be at least 64KB (65536 bits)
    for (int i = 0; i < frameBufferCount; ++i)
    {
        // create resource for cube 1
        ASSERT_SUCCEEDED(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // this heap will be used to upload the constant buffer data
            D3D12_HEAP_FLAG_NONE, // no flags
            &CD3DX12_RESOURCE_DESC::Buffer(1024 * 64), // size of the resource heap. Must be a multiple of 64KB for single-textures and constant buffers
            D3D12_RESOURCE_STATE_GENERIC_READ, // will be data that is read from so we keep it in the generic read state
            nullptr, // we do not have use an optimized clear value for constant buffers
            IID_PPV_ARGS(&constantBufferUploadHeaps[i])));
        constantBufferUploadHeaps[i]->SetName(L"Constant Buffer Upload Resource Heap");
    }
}

void DXSample::CreateRootSignature()
{
    // create a root descriptor, which explains where to find the data for this root parameter
    D3D12_ROOT_DESCRIPTOR rootCBVDescriptor;
    rootCBVDescriptor.RegisterSpace = 0;
    rootCBVDescriptor.ShaderRegister = 0;

    // create a root parameter and fill it out
    D3D12_ROOT_PARAMETER rootParameters[2]; // only one parameter right now
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // this is a constant buffer view root descriptor
    rootParameters[0].Descriptor = rootCBVDescriptor; // this is the root descriptor for this root parameter
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // our pixel shader will be the only shader accessing this parameter for now

    // create a descriptor range (descriptor table) and fill it out
    // this is a range of descriptors inside a descriptor heap
    D3D12_DESCRIPTOR_RANGE descriptorTableRangesTexture[1];
    descriptorTableRangesTexture[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // this is a range of shader resource views (descriptors)
    descriptorTableRangesTexture[0].NumDescriptors = max_texture_slot; // we only have one texture right now, so the range is only 1
    descriptorTableRangesTexture[0].BaseShaderRegister = 0; // start index of the shader registers in the range
    descriptorTableRangesTexture[0].RegisterSpace = 0; // space 0. can usually be zero
    descriptorTableRangesTexture[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // this appends the range to the end of the root signature descriptor tables

    // create a descriptor table
    D3D12_ROOT_DESCRIPTOR_TABLE descriptorTableTexture;
    descriptorTableTexture.NumDescriptorRanges = _countof(descriptorTableRangesTexture); // we only have one range
    descriptorTableTexture.pDescriptorRanges = &descriptorTableRangesTexture[0]; // the pointer to the beginning of our ranges array

    // create a root parameter and fill it out
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // this is a descriptor table
    rootParameters[1].DescriptorTable = descriptorTableTexture; // this is our descriptor table for this root parameter
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // our pixel shader will be the only shader accessing this parameter for now

    // create a static sampler
    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampler.MipLODBias = 0;
    sampler.MaxAnisotropy = 0;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    sampler.MinLOD = 0.0f;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister = 0;
    sampler.RegisterSpace = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // we can deny shader stages here for better performance
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(_countof(rootParameters), // we have 1 root parameter
        rootParameters, // a pointer to the beginning of our root parameters array
        1,
        &sampler,
        rootSignatureFlags);

    ID3DBlob* signature;
    ID3DBlob* errorBuff; // a buffer holding the error data if any
    ASSERT_SUCCEEDED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &errorBuff),
        "%s", (char*)errorBuff->GetBufferPointer());
    ASSERT_SUCCEEDED(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
}

void DXSample::CreatePSO()
{
    // create vertex and pixel shaders

    // when debugging, we can compile the shader files at runtime.
    // but for release versions, we can compile the hlsl shaders
    // with fxc.exe to create .cso files, which contain the shader
    // bytecode. We can load the .cso files at runtime to get the
    // shader bytecode, which of course is faster than compiling
    // them at runtime

    // compile vertex shader
    ID3DBlob* vertexShader; // d3d blob for holding vertex shader bytecode
    ID3DBlob* errorBuff; // a buffer holding the error data if any
    ASSERT_SUCCEEDED(D3DCompileFromFile(GetAssetFullPath(L"shaders/DX12/VertexShader.hlsl").c_str(),
        nullptr,
        nullptr,
        "main",
        "vs_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0,
        &vertexShader,
        &errorBuff),
        "%s", (char*)errorBuff->GetBufferPointer());

    // fill out a shader bytecode structure, which is basically just a pointer
    // to the shader bytecode and the size of the shader bytecode
    D3D12_SHADER_BYTECODE vertexShaderBytecode = {};
    vertexShaderBytecode.BytecodeLength = vertexShader->GetBufferSize();
    vertexShaderBytecode.pShaderBytecode = vertexShader->GetBufferPointer();

    // compile pixel shader
    ID3DBlob* pixelShader;
    ASSERT_SUCCEEDED(D3DCompileFromFile(GetAssetFullPath(L"shaders/DX12/PixelShader.hlsl").c_str(),
        nullptr,
        nullptr,
        "main",
        "ps_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0,
        &pixelShader,
        &errorBuff),
        "%s", (char*)errorBuff->GetBufferPointer());

    // fill out shader bytecode structure for pixel shader
    D3D12_SHADER_BYTECODE pixelShaderBytecode = {};
    pixelShaderBytecode.BytecodeLength = pixelShader->GetBufferSize();
    pixelShaderBytecode.pShaderBytecode = pixelShader->GetBufferPointer();

    // create input layout

    // The input layout is used by the Input Assembler so that it knows
    // how to read the vertex data bound to it.
    D3D12_INPUT_ELEMENT_DESC inputLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(DX12Mesh::Vertex, position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(DX12Mesh::Vertex, normal), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(DX12Mesh::Vertex, texCoords), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(DX12Mesh::Vertex, tangent), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(DX12Mesh::Vertex, bitangent), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    // fill out an input layout description structure
    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};

    // we can get the number of elements in an array by "sizeof(array) / sizeof(arrayElementType)"
    inputLayoutDesc.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
    inputLayoutDesc.pInputElementDescs = inputLayout;


    // create a pipeline state object (PSO)

    // In a real application, you will have many pso's. for each different shader
    // or different combinations of shaders, different blend states or different rasterizer states,
    // different topology types (point, line, triangle, patch), or a different number
    // of render targets you will need a pso

    // VS is the only required shader for a pso. You might be wondering when a case would be where
    // you only set the VS. It's possible that you have a pso that only outputs data with the stream
    // output, and not on a render target, which means you would not need anything after the stream
    // output.

    DXGI_SAMPLE_DESC sampleDesc = {};
    sampleDesc.Count = 1; // multisample count (no multisampling, so we just put 1, since we still need 1 sample)

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {}; // a structure to define a pso
    psoDesc.InputLayout = inputLayoutDesc; // the structure describing our input layout
    psoDesc.pRootSignature = rootSignature.Get(); // the root signature that describes the input data this pso needs
    psoDesc.VS = vertexShaderBytecode; // structure describing where to find the vertex shader bytecode and how large it is
    psoDesc.PS = pixelShaderBytecode; // same as VS but for pixel shader
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // type of topology we are drawing
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // format of the render target
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT; // format of the DS
    psoDesc.SampleDesc = sampleDesc; // must be the same sample description as the swapchain and depth/stencil buffer
    psoDesc.SampleMask = UINT_MAX; // sample mask has to do with multi-sampling. 0xffffffff means point sampling is done
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); // a default rasterizer state.
    D3D12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = blendDesc; // a default blent state.
    psoDesc.NumRenderTargets = 1; // we are only binding one render target
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // a default depth stencil state

    // create the pso
    ASSERT_SUCCEEDED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineStateObject)));

    blendDesc.RenderTarget[0].BlendEnable = true;
    blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
    psoDesc.BlendState = blendDesc; // a default blent state.
    ASSERT_SUCCEEDED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineStateObjectWithBlend)));
}

void DXSample::CreateViewPort()
{
    // Fill out the Viewport
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = (FLOAT)m_width;
    viewport.Height = (FLOAT)m_height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    // Fill out a scissor rect
    scissorRect.left = 0;
    scissorRect.top = 0;
    scissorRect.right = m_width;
    scissorRect.bottom = m_height;
}

void DXSample::CreateTexture(TextureInfo& texture, DX12Mesh::TexResources& texResources)
{
    TexInfo texInfo = LoadImageDataFromFile(texture.path);

    // create a default heap where the upload heap will copy its contents into (contents being the texture)
    ASSERT_SUCCEEDED(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), // a default heap
        D3D12_HEAP_FLAG_NONE, // no flags
        &texInfo.resourceDescription, // the description of our texture
        D3D12_RESOURCE_STATE_COPY_DEST, // We will copy the texture from the upload heap to here, so we start it out in a copy dest state
        nullptr, // used for render targets and depth/stencil buffers
        IID_PPV_ARGS(&texResources.buffer.defaultHeap)));
    texResources.buffer.defaultHeap->SetName(L"Texture Buffer Resource Heap");

    UINT64 textureUploadBufferSize;
    // this function gets the size an upload buffer needs to be to upload a texture to the gpu.
    // each row must be 256 byte aligned except for the last row, which can just be the size in bytes of the row
    // eg. textureUploadBufferSize = ((((width * numBytesPerPixel) + 255) & ~255) * (height - 1)) + (width * numBytesPerPixel);
    //textureUploadBufferSize = (((imageBytesPerRow + 255) & ~255) * (textureDesc.Height - 1)) + imageBytesPerRow;
    device->GetCopyableFootprints(&texInfo.resourceDescription, 0, 1, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);

    // now we create an upload heap to upload our texture to the GPU
    ASSERT_SUCCEEDED(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // upload heap
        D3D12_HEAP_FLAG_NONE, // no flags
        &CD3DX12_RESOURCE_DESC::Buffer(textureUploadBufferSize), // resource description for a buffer (storing the image data in this heap just to copy to the default heap)
        D3D12_RESOURCE_STATE_GENERIC_READ, // We will copy the contents from this heap to the default heap above
        nullptr,
        IID_PPV_ARGS(&texResources.buffer.uploadHeap)));
    texResources.buffer.uploadHeap->SetName(L"Texture Buffer Upload Resource Heap");

    // store vertex buffer in upload heap
    D3D12_SUBRESOURCE_DATA textureData = {};
    textureData.pData = &texInfo.imageData[0]; // pointer to our image data
    textureData.RowPitch = texInfo.bytesPerRow; // size of all our triangle vertex data
    textureData.SlicePitch = texInfo.bytesPerRow * texInfo.textureHeight; // also the size of our triangle vertex data

    // Now we copy the upload buffer contents to the default heap
    UpdateSubresources(commandList.Get(), texResources.buffer.defaultHeap.Get(), texResources.buffer.uploadHeap.Get(), 0, 0, 1, &textureData);

    // transition the texture default heap to a pixel shader resource (we will be sampling from this heap in the pixel shader to get the color of pixels)
    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texResources.buffer.defaultHeap.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

    // now we create a shader resource view (descriptor that points to the texture and describes it)
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = texInfo.resourceDescription.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    const UINT cbvSrvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE cbvSrvHeapStart = mainDescriptorTextureHeap->GetCPUDescriptorHandleForHeapStart();
    CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle(cbvSrvHeapStart, texResources.offset, cbvSrvDescriptorSize);
    device->CreateShaderResourceView(texResources.buffer.defaultHeap.Get(), &srvDesc, cbvSrvHandle);
}

void DXSample::UploadAllResources()
{
    // Now we execute the command list to upload the initial assets (triangle data)
    commandList->Close();
    ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
    commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // increment the fence value now, otherwise the buffer might not be uploaded by the time we start drawing
    fenceValue[frameIndex]++;
    ASSERT_SUCCEEDED(commandQueue->Signal(fence[frameIndex].Get(), fenceValue[frameIndex]));
}

DXSample::TexInfo DXSample::LoadImageDataFromFile(std::string filename)
{
    TexInfo texInfo = {};

    unsigned char *image = SOIL_load_image(filename.c_str(), &texInfo.textureWidth, &texInfo.textureHeight, 0, SOIL_LOAD_RGBA);

    DXGI_FORMAT dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

    size_t num_bytes;
    size_t row_bytes;
    GetSurfaceInfo(texInfo.textureWidth, texInfo.textureHeight, dxgiFormat, &num_bytes, &row_bytes, nullptr);
    texInfo.bytesPerRow = row_bytes;
    texInfo.imageSize = num_bytes;
    texInfo.imageData.reset(new uint8_t[texInfo.imageSize]);
    memcpy(texInfo.imageData.get(), image, texInfo.imageSize);
    SOIL_free_image_data(image);

    // now describe the texture with the information we have obtained from the image
    texInfo.resourceDescription = {};
    texInfo.resourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texInfo.resourceDescription.Alignment = 0; // may be 0, 4KB, 64KB, or 4MB. 0 will let runtime decide between 64KB and 4MB (4MB for multi-sampled textures)
    texInfo.resourceDescription.Width = texInfo.textureWidth; // width of the texture
    texInfo.resourceDescription.Height = texInfo.textureHeight; // height of the texture
    texInfo.resourceDescription.DepthOrArraySize = 1; // if 3d image, depth of 3d image. Otherwise an array of 1D or 2D textures (we only have one image, so we set 1)
    texInfo.resourceDescription.MipLevels = 1; // Number of mipmaps. We are not generating mipmaps for this texture, so we have only one level
    texInfo.resourceDescription.Format = dxgiFormat; // This is the dxgi format of the image (format of the pixels)
    texInfo.resourceDescription.SampleDesc.Count = 1; // This is the number of samples per pixel, we just want 1 sample
    texInfo.resourceDescription.SampleDesc.Quality = 0; // The quality level of the samples. Higher is better quality, but worse performance
    texInfo.resourceDescription.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // The arrangement of the pixels. Setting to unknown lets the driver choose the most efficient one
    texInfo.resourceDescription.Flags = D3D12_RESOURCE_FLAG_NONE; // no flags
    return texInfo;
}

void DXSample::WaitForPreviousFrame()
{
    // if the current fence value is still less than "fenceValue", then we know the GPU has not finished executing
    // the command queue since it has not reached the "commandQueue->Signal(fence, fenceValue)" command
    if (fence[frameIndex]->GetCompletedValue() < fenceValue[frameIndex])
    {
        // we have the fence create an event which is signaled once the fence's current value is "fenceValue"
        ASSERT_SUCCEEDED(fence[frameIndex]->SetEventOnCompletion(fenceValue[frameIndex], fenceEvent));

        // We will wait until the fence has triggered the event that it's current value has reached "fenceValue". once it's value
        // has reached "fenceValue", we know the command queue has finished executing
        WaitForSingleObject(fenceEvent, INFINITE);
    }

    // increment fenceValue for next frame
    fenceValue[frameIndex]++;
}

void DXSample::PopulateCommandList()
{
    // swap the current rtv buffer index so we draw on the correct buffer
    frameIndex = swapChain->GetCurrentBackBufferIndex();
    // We have to wait for the gpu to finish with the command allocator before we reset it
    WaitForPreviousFrame();

    // we can only reset an allocator once the gpu is done with it
    // resetting an allocator frees the memory that the command list was stored in
    ASSERT_SUCCEEDED(commandAllocator[frameIndex]->Reset());

    // reset the command list. by resetting the command list we are putting it into
    // a recording state so we can start recording commands into the command allocator.
    // the command allocator that we reference here may have multiple command lists
    // associated with it, but only one can be recording at any time. Make sure
    // that any other command lists associated to this command allocator are in
    // the closed state (not recording).
    // Here you will pass an initial pipeline state object as the second parameter,
    // but in this tutorial we are only clearing the rtv, and do not actually need
    // anything but an initial default pipeline, which is what we get by setting
    // the second parameter to NULL
    ASSERT_SUCCEEDED(commandList->Reset(commandAllocator[frameIndex].Get(), pipelineStateObject.Get()));

    // here we start recording commands into the commandList (which all the commands will be stored in the commandAllocator)

    // transition the "frameIndex" render target from the present state to the render target state so the command list draws to it starting from here
    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    // here we again get the handle to our current render target view so we can set it as the render target in the output merger stage of the pipeline
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, rtvDescriptorSize);

    // get a handle to the depth/stencil buffer
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    // set the render target for the output merger stage (the output of the pipeline)
    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    // Clear the render target by using the ClearRenderTargetView command
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    // clear the depth/stencil buffer
    commandList->ClearDepthStencilView(dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // set root signature
    commandList->SetGraphicsRootSignature(rootSignature.Get()); // set the root signature

    // draw triangle
    commandList->RSSetViewports(1, &viewport); // set the viewports
    commandList->RSSetScissorRects(1, &scissorRect); // set the scissor rects
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // set the primitive topology
    commandList->SetGraphicsRootConstantBufferView(0, constantBufferUploadHeaps[frameIndex]->GetGPUVirtualAddress());

    for (size_t mesh_id = 0; mesh_id < m_modelOfFile->meshes.size(); ++mesh_id)
    {
        DX12Mesh & cur_mesh = m_modelOfFile->meshes[mesh_id];

        if (cur_mesh.material.name.C_Str() == std::string("Ground_SG"))
            continue;

        if (cur_mesh.material.name.C_Str() == std::string("Windows_SG"))
            commandList->SetPipelineState(pipelineStateObjectWithBlend.Get());
        else
            commandList->SetPipelineState(pipelineStateObject.Get());

        commandList->IASetVertexBuffers(0, 1, &cur_mesh.vertexBufferView);
        commandList->IASetIndexBuffer(&cur_mesh.indexBufferView);

        const UINT cbvSrvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        static uint32_t frameId = 0;
        for (size_t i = 0; i < cur_mesh.textures.size(); ++i)
        {
            uint32_t texture_slot = 0;
            switch (cur_mesh.textures[i].type)
            {
            case aiTextureType_DIFFUSE:
                texture_slot = 0;
                break;
            case aiTextureType_HEIGHT:
                texture_slot = 1;
                break;
            case aiTextureType_SPECULAR:
                texture_slot = 2;
                break;
            case aiTextureType_SHININESS:
                texture_slot = 3;
                break;
            case aiTextureType_AMBIENT:
                texture_slot = 4;
                break;
            default:
                continue;
            }

            CD3DX12_CPU_DESCRIPTOR_HANDLE srcTextureHandle(mainDescriptorTextureHeap->GetCPUDescriptorHandleForHeapStart(), cur_mesh.texResources[i].offset, cbvSrvDescriptorSize);
            CD3DX12_CPU_DESCRIPTOR_HANDLE dstTextureHandle(cur_mesh.currentDescriptorTextureHeap->GetCPUDescriptorHandleForHeapStart(), texture_slot, cbvSrvDescriptorSize);

            device->CopyDescriptors(
                1, &dstTextureHandle, nullptr,
                1, &srcTextureHandle, nullptr,
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }

        ID3D12DescriptorHeap *descriptorHeaps[] = { cur_mesh.currentDescriptorTextureHeap.Get() };
        commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
        commandList->SetGraphicsRootDescriptorTable(1, cur_mesh.currentDescriptorTextureHeap->GetGPUDescriptorHandleForHeapStart());
        commandList->DrawIndexedInstanced(cur_mesh.indices.size(), 1, 0, 0, 0);
    }

    // transition the "frameIndex" render target from the render target state to the present state. If the debug layer is enabled, you will receive a
    // warning if present is called on the render target when it's not in the present state
    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    ASSERT_SUCCEEDED(commandList->Close());
}

void DXSample::OnInit(int width, int height)
{
    m_width = width;
    m_height = height;

    CreateDevice();
    CreateCommandQueue();
    CreateSwapChain();
    CreateFence();
    CreateRT();
    CreateDepthStencil();

    CreateCommandAllocators();
    CreateCommandList();
    CreateGeometry();
    CreateConstantBuffer();
    CreateRootSignature();
    CreatePSO();
    CreateViewPort();
    UploadAllResources();
}

void DXSample::OnUpdate()
{
    static std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
    static std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
    static float angle = 0.0;

    end = std::chrono::system_clock::now();
    int64_t elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    start = std::chrono::system_clock::now();

    if (use_rotare)
        angle += elapsed / 2e6f;

    float z_width = (m_modelOfFile->bound_box.z_max - m_modelOfFile->bound_box.z_min);
    float y_width = (m_modelOfFile->bound_box.y_max - m_modelOfFile->bound_box.y_min);
    float x_width = (m_modelOfFile->bound_box.y_max - m_modelOfFile->bound_box.y_min);
    float model_width = (z_width + y_width + x_width) / 3.0f;
    float scale = 256.0f / std::max(z_width, x_width);
    model_width *= scale;

    float offset_x = (m_modelOfFile->bound_box.x_max + m_modelOfFile->bound_box.x_min) / 2.0f;
    float offset_y = (m_modelOfFile->bound_box.y_max + m_modelOfFile->bound_box.y_min) / 2.0f;
    float offset_z = (m_modelOfFile->bound_box.z_max + m_modelOfFile->bound_box.z_min) / 2.0f;

    cameraPosition = glm::vec3(0.0f, model_width * 0.25f, -model_width * 2.0f);
    cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

    glm::mat4 model = glm::rotate(-angle, glm::vec3(0.0, 1.0, 0.0)) * glm::translate(glm::vec3(-offset_x, -offset_y, -offset_z)) * glm::scale(glm::vec3(scale, scale, scale));
    glm::mat4 view = glm::lookAtRH(cameraPosition, cameraTarget, cameraUp);
    glm::mat4 proj = glm::perspectiveFovRH(45.0f * (3.14f / 180.0f), (float)m_width, (float)m_height, 0.1f, 1024.0f);

    cbPerObject.model = glm::transpose(model);
    cbPerObject.view = glm::transpose(view);
    cbPerObject.projection = glm::transpose(proj);
    cbPerObject.lightPos = glm::vec4(cameraPosition, 0.0);
    cbPerObject.viewPos = glm::vec4(cameraPosition, 0.0);

    CD3DX12_RANGE readRange(0, 0);
    uint8_t* cbvGPUAddress = nullptr;
    ASSERT_SUCCEEDED(constantBufferUploadHeaps[frameIndex]->Map(0, &readRange, reinterpret_cast<void**>(&cbvGPUAddress)));
    memcpy(cbvGPUAddress, &cbPerObject, sizeof(cbPerObject));
    constantBufferUploadHeaps[frameIndex]->Unmap(0, &readRange);
}

void DXSample::OnRender()
{
    PopulateCommandList(); // update the pipeline by sending commands to the commandqueue

    // create an array of command lists (only one command list here)
    ID3D12CommandList* ppCommandLists[] = { commandList.Get() };

    // execute the array of command lists
    commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // this command goes in at the end of our command queue. we will know when our command queue
    // has finished because the fence value will be set to "fenceValue" from the GPU since the command
    // queue is being executed on the GPU
    ASSERT_SUCCEEDED(commandQueue->Signal(fence[frameIndex].Get(), fenceValue[frameIndex]));

    // present the current backbuffer
    ASSERT_SUCCEEDED(swapChain->Present(0, 0));
}

void DXSample::OnDestroy()
{
    WaitForGpu();

    // get swapchain out of full screen before exiting
    BOOL is_fullscreen = false;
    ASSERT_SUCCEEDED(swapChain->GetFullscreenState(&is_fullscreen, nullptr));
    if (is_fullscreen)
        swapChain->SetFullscreenState(false, nullptr);

    // close the fence event
    CloseHandle(fenceEvent);
}

void DXSample::WaitForGpu()
{
    frameIndex = swapChain->GetCurrentBackBufferIndex();
    for (int i = 0; i < frameBufferCount; ++i)
    {
        WaitForPreviousFrame();
        commandQueue->Signal(fence[frameIndex].Get(), fenceValue[frameIndex]);
        frameIndex = (frameIndex + 1) % frameBufferCount;
    }
}

void DXSample::OnSizeChanged(int width, int height)
{
    if ((width != m_width || height != m_height))
    {
        m_width = width;
        m_height = height;

        WaitForGpu();
        for (int i = 0; i < frameBufferCount; ++i)
        {
            renderTargets[i].Reset();
        }

        DXGI_SWAP_CHAIN_DESC desc = {};
        swapChain->GetDesc(&desc);
        ASSERT_SUCCEEDED(swapChain->ResizeBuffers(frameBufferCount, width, height, desc.BufferDesc.Format, desc.Flags));
        frameIndex = swapChain->GetCurrentBackBufferIndex();

        CreateRT();
        CreateDepthStencil();
        CreateViewPort();
    }
}
