#include "DXSample.h"
#include "Utility.h"
#include "FileUtility.h"

DXSample::DXSample(UINT width, UINT height) :
    m_width(width),
    m_height(height)
{
}

DXSample::~DXSample()
{
}

void DXSample::CreateDevice()
{
    ASSERT_SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)));

    auto GetHardwareAdapter = [&](IDXGIFactory4* dxgiFactory) -> IDXGIAdapter1*
    {
        IDXGIAdapter1* adapter; // adapters are the graphics card (this includes the embedded graphics on the motherboard)
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
            if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_1, _uuidof(ID3D12Device), nullptr)))
                return adapter;
        }
        return nullptr;
    };

    IDXGIAdapter1* adapter = GetHardwareAdapter(dxgiFactory);
    ASSERT(adapter != nullptr);
    ASSERT_SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS(&device)));
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
    swapChainDesc.OutputWindow = Win32Application::GetHwnd(); // handle to our window
    swapChainDesc.SampleDesc = sampleDesc; // our multi-sampling description
    swapChainDesc.Windowed = true; // set to true, then if in fullscreen must call SetFullScreenState with true for full screen to get uncapped fps

    IDXGISwapChain* tempSwapChain;
    ASSERT_SUCCEEDED(dxgiFactory->CreateSwapChain(
        commandQueue, // the queue will be flushed once the swap chain is created
        &swapChainDesc, // give it the swap chain description we created above
        &tempSwapChain // store the created swap chain in a temp IDXGISwapChain interface
        ));

    swapChain = static_cast<IDXGISwapChain3*>(tempSwapChain);
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
        device->CreateRenderTargetView(renderTargets[i], nullptr, rtvHandle);

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

    device->CreateDepthStencilView(depthStencilBuffer, &depthStencilDesc, dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
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
    ASSERT_SUCCEEDED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator[frameIndex], NULL, IID_PPV_ARGS(&commandList)));
}

void DXSample::CreateVertex()
{
    Vertex vList[] = {
        // front face
        { -0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
        { 0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
        { -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
        { 0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f },

        // right side face
        { 0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
        { 0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
        { 0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
        { 0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f },

        // left side face
        { -0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
        { -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
        { -0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
        { -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f },

        // back face
        { 0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
        { -0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
        { 0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
        { -0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f },

        // top face
        { -0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
        { 0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
        { 0.5f,  0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
        { -0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f },

        // bottom face
        { 0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
        { -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
        { 0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
        { -0.5f, -0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
    };

    int vBufferSize = sizeof(vList);

    // create default heap
    // default heap is memory on the GPU. Only the GPU has access to this memory
    // To get data into this heap, we will have to upload the data using
    // an upload heap
    device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), // a default heap
        D3D12_HEAP_FLAG_NONE, // no flags
        &CD3DX12_RESOURCE_DESC::Buffer(vBufferSize), // resource description for a buffer
        D3D12_RESOURCE_STATE_COPY_DEST, // we will start this heap in the copy destination state since we will copy data
                                        // from the upload heap to this heap
        nullptr, // optimized clear value must be null for this type of resource. used for render targets and depth/stencil buffers
        IID_PPV_ARGS(&vertexBuffer));

    // we can give resource heaps a name so when we debug with the graphics debugger we know what resource we are looking at
    vertexBuffer->SetName(L"Vertex Buffer Resource Heap");

    // create upload heap
    // upload heaps are used to upload data to the GPU. CPU can write to it, GPU can read from it
    // We will upload the vertex buffer using this heap to the default heap
    ID3D12Resource* vBufferUploadHeap;
    device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // upload heap
        D3D12_HEAP_FLAG_NONE, // no flags
        &CD3DX12_RESOURCE_DESC::Buffer(vBufferSize), // resource description for a buffer
        D3D12_RESOURCE_STATE_GENERIC_READ, // GPU will read from this buffer and copy its contents to the default heap
        nullptr,
        IID_PPV_ARGS(&vBufferUploadHeap));
    vBufferUploadHeap->SetName(L"Vertex Buffer Upload Resource Heap");

    // store vertex buffer in upload heap
    D3D12_SUBRESOURCE_DATA vertexData = {};
    vertexData.pData = reinterpret_cast<BYTE*>(vList); // pointer to our vertex array
    vertexData.RowPitch = vBufferSize; // size of all our triangle vertex data
    vertexData.SlicePitch = vBufferSize; // also the size of our triangle vertex data

    // we are now creating a command with the command list to copy the data from
    // the upload heap to the default heap
    UpdateSubresources(commandList, vertexBuffer, vBufferUploadHeap, 0, 0, 1, &vertexData);

    // transition the vertex buffer data from copy destination state to vertex buffer state
    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

    // Create index buffer

    // a quad (2 triangles)
    DWORD iList[] = {
        // ffront face
        0, 1, 2, // first triangle
        0, 3, 1, // second triangle

                 // left face
        4, 5, 6, // first triangle
        4, 7, 5, // second triangle

                 // right face
        8, 9, 10, // first triangle
        8, 11, 9, // second triangle

                  // back face
        12, 13, 14, // first triangle
        12, 15, 13, // second triangle

                    // top face
        16, 17, 18, // first triangle
        16, 19, 17, // second triangle

                    // bottom face
        20, 21, 22, // first triangle
        20, 23, 21, // second triangle
    };

    int iBufferSize = sizeof(iList);

    numCubeIndices = sizeof(iList) / sizeof(DWORD);

    // create default heap to hold index buffer
    device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), // a default heap
        D3D12_HEAP_FLAG_NONE, // no flags
        &CD3DX12_RESOURCE_DESC::Buffer(iBufferSize), // resource description for a buffer
        D3D12_RESOURCE_STATE_COPY_DEST, // start in the copy destination state
        nullptr, // optimized clear value must be null for this type of resource
        IID_PPV_ARGS(&indexBuffer));

    // we can give resource heaps a name so when we debug with the graphics debugger we know what resource we are looking at
    vertexBuffer->SetName(L"Index Buffer Resource Heap");

    // create upload heap to upload index buffer
    ID3D12Resource* iBufferUploadHeap;
    device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // upload heap
        D3D12_HEAP_FLAG_NONE, // no flags
        &CD3DX12_RESOURCE_DESC::Buffer(iBufferSize), // resource description for a buffer
        D3D12_RESOURCE_STATE_GENERIC_READ, // GPU will read from this buffer and copy its contents to the default heap
        nullptr,
        IID_PPV_ARGS(&iBufferUploadHeap));
    iBufferUploadHeap->SetName(L"Index Buffer Upload Resource Heap");

    // store vertex buffer in upload heap
    D3D12_SUBRESOURCE_DATA indexData = {};
    indexData.pData = reinterpret_cast<BYTE*>(iList); // pointer to our index array
    indexData.RowPitch = iBufferSize; // size of all our index buffer
    indexData.SlicePitch = iBufferSize; // also the size of our index buffer

    // we are now creating a command with the command list to copy the data from
    // the upload heap to the default heap
    UpdateSubresources(commandList, indexBuffer, iBufferUploadHeap, 0, 0, 1, &indexData);

    // transition the vertex buffer data from copy destination state to vertex buffer state
    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(indexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

    // Now we execute the command list to upload the initial assets (triangle data)
    commandList->Close();
    ID3D12CommandList* ppCommandLists[] = { commandList };
    commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // increment the fence value now, otherwise the buffer might not be uploaded by the time we start drawing
    fenceValue[frameIndex]++;
    ASSERT_SUCCEEDED(commandQueue->Signal(fence[frameIndex], fenceValue[frameIndex]));

    // create a vertex buffer view for the triangle. We get the GPU memory address to the vertex pointer using the GetGPUVirtualAddress() method
    vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vertexBufferView.StrideInBytes = sizeof(Vertex);
    vertexBufferView.SizeInBytes = vBufferSize;

    // create a vertex buffer view for the triangle. We get the GPU memory address to the vertex pointer using the GetGPUVirtualAddress() method
    indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
    indexBufferView.Format = DXGI_FORMAT_R32_UINT; // 32-bit unsigned integer (this is what a dword is, double word, a word is 2 bytes)
    indexBufferView.SizeInBytes = iBufferSize;
}

void DXSample::CreateConstantBuffer()
{
    // Create a constant buffer descriptor heap for each frame
    // this is the descriptor heap that will store our constant buffer descriptor
    for (int i = 0; i < frameBufferCount; ++i)
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = 1;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        ASSERT_SUCCEEDED(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mainDescriptorHeap[i])));
    }

    // create the constant buffer resource heap
    // We will update the constant buffer one or more times per frame, so we will use only an upload heap
    // unlike previously we used an upload heap to upload the vertex and index data, and then copied over
    // to a default heap. If you plan to use a resource for more than a couple frames, it is usually more
    // efficient to copy to a default heap where it stays on the gpu. In this case, our constant buffer
    // will be modified and uploaded at least once per frame, so we only use an upload heap

    // create a resource heap, descriptor heap, and pointer to cbv for each frame
    for (int i = 0; i < frameBufferCount; ++i)
    {
        ASSERT_SUCCEEDED(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // this heap will be used to upload the constant buffer data
            D3D12_HEAP_FLAG_NONE, // no flags
            &CD3DX12_RESOURCE_DESC::Buffer(1024 * 64), // size of the resource heap. Must be a multiple of 64KB for single-textures and constant buffers
            D3D12_RESOURCE_STATE_GENERIC_READ, // will be data that is read from so we keep it in the generic read state
            nullptr, // we do not have use an optimized clear value for constant buffers
            IID_PPV_ARGS(&constantBufferUploadHeap[i])));
        constantBufferUploadHeap[i]->SetName(L"Constant Buffer Upload Resource Heap");

        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = constantBufferUploadHeap[i]->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = (sizeof(ConstantBuffer) + 255) & ~255;    // CB size is required to be 256-byte aligned.
        device->CreateConstantBufferView(&cbvDesc, mainDescriptorHeap[i]->GetCPUDescriptorHandleForHeapStart());
    }

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
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // our pixel shader will be the only shader accessing this parameter for now


    // create a descriptor range (descriptor table) and fill it out
    // this is a range of descriptors inside a descriptor heap
    D3D12_DESCRIPTOR_RANGE  descriptorTableRanges[1]; // only one range right now
    descriptorTableRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; // this is a range of constant buffer views (descriptors)
    descriptorTableRanges[0].NumDescriptors = 1; // we only have one constant buffer, so the range is only 1
    descriptorTableRanges[0].BaseShaderRegister = 1; // start index of the shader registers in the range
    descriptorTableRanges[0].RegisterSpace = 0; // space 0. can usually be zero
    descriptorTableRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // this appends the range to the end of the root signature descriptor tables

    // create a descriptor table
    D3D12_ROOT_DESCRIPTOR_TABLE descriptorTable;
    descriptorTable.NumDescriptorRanges = _countof(descriptorTableRanges); // we only have one range
    descriptorTable.pDescriptorRanges = &descriptorTableRanges[0]; // the pointer to the beginning of our ranges array

    // create a root parameter and fill it out
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // this is a descriptor table
    rootParameters[1].DescriptorTable = descriptorTable; // this is our descriptor table for this root parameter
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // our pixel shader will be the only shader accessing this parameter for now


    // Allow input layout and deny uneccessary access to certain pipeline stages.
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(_countof(rootParameters), // we have 1 root parameter
        rootParameters, // a pointer to the beginning of our root parameters array
        0,
        nullptr,
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
    ASSERT_SUCCEEDED(D3DCompileFromFile(GetAssetFullPath(L"shaders/VertexShader.hlsl").c_str(),
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
    ASSERT_SUCCEEDED(D3DCompileFromFile(GetAssetFullPath(L"shaders/PixelShader.hlsl").c_str(),
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
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
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
    psoDesc.pRootSignature = rootSignature; // the root signature that describes the input data this pso needs
    psoDesc.VS = vertexShaderBytecode; // structure describing where to find the vertex shader bytecode and how large it is
    psoDesc.PS = pixelShaderBytecode; // same as VS but for pixel shader
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // type of topology we are drawing
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // format of the render target
    psoDesc.SampleDesc = sampleDesc; // must be the same sample description as the swapchain and depth/stencil buffer
    psoDesc.SampleMask = UINT_MAX; // sample mask has to do with multi-sampling. 0xffffffff means point sampling is done
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); // a default rasterizer state.
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT); // a default blent state.
    psoDesc.NumRenderTargets = 1; // we are only binding one render target
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // a default depth stencil state

    // create the pso
    ASSERT_SUCCEEDED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineStateObject)));
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

void DXSample::CreateMatrix()
{
    // build projection and view matrix
    XMMATRIX tmpMat = XMMatrixPerspectiveFovLH(45.0f*(3.14f / 180.0f), (float)m_width / (float)m_height, 0.1f, 1000.0f);
    XMStoreFloat4x4(&cameraProjMat, tmpMat);

    // set starting camera state
    cameraPosition = XMFLOAT4(0.0f, 2.0f, -4.0f, 0.0f);
    cameraTarget = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
    cameraUp = XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);

    // build view matrix
    XMVECTOR cPos = XMLoadFloat4(&cameraPosition);
    XMVECTOR cTarg = XMLoadFloat4(&cameraTarget);
    XMVECTOR cUp = XMLoadFloat4(&cameraUp);
    tmpMat = XMMatrixLookAtLH(cPos, cTarg, cUp);
    XMStoreFloat4x4(&cameraViewMat, tmpMat);

    // set starting cubes position
    // first cube
    cube1Position = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f); // set cube 1's position
    XMVECTOR posVec = XMLoadFloat4(&cube1Position); // create xmvector for cube1's position

    tmpMat = XMMatrixTranslationFromVector(posVec); // create translation matrix from cube1's position vector
    XMStoreFloat4x4(&cube1RotMat, XMMatrixIdentity()); // initialize cube1's rotation matrix to identity matrix
    XMStoreFloat4x4(&cube1WorldMat, tmpMat); // store cube1's world matrix

                                             // second cube
    cube2PositionOffset = XMFLOAT4(1.5f, 0.0f, 0.0f, 0.0f);
    posVec = XMLoadFloat4(&cube2PositionOffset) + XMLoadFloat4(&cube1Position); // create xmvector for cube2's position
                                                                                // we are rotating around cube1 here, so add cube2's position to cube1

    tmpMat = XMMatrixTranslationFromVector(posVec); // create translation matrix from cube2's position offset vector
    XMStoreFloat4x4(&cube2RotMat, XMMatrixIdentity()); // initialize cube2's rotation matrix to identity matrix
    XMStoreFloat4x4(&cube2WorldMat, tmpMat); // store cube2's world matrix
}

void DXSample::WaitForPreviousFrame()
{
    // swap the current rtv buffer index so we draw on the correct buffer
    frameIndex = swapChain->GetCurrentBackBufferIndex();

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
    ASSERT_SUCCEEDED(commandList->Reset(commandAllocator[frameIndex], pipelineStateObject));

    // here we start recording commands into the commandList (which all the commands will be stored in the commandAllocator)

    // transition the "frameIndex" render target from the present state to the render target state so the command list draws to it starting from here
    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

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
    commandList->SetGraphicsRootSignature(rootSignature); // set the root signature

    // set constant buffer descriptor heap
    ID3D12DescriptorHeap* descriptorHeaps[] = { mainDescriptorHeap[frameIndex] };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    // set the root descriptor table 0 to the constant buffer descriptor heap
    commandList->SetGraphicsRootDescriptorTable(1, mainDescriptorHeap[frameIndex]->GetGPUDescriptorHandleForHeapStart());

    // draw triangle
    commandList->RSSetViewports(1, &viewport); // set the viewports
    commandList->RSSetScissorRects(1, &scissorRect); // set the scissor rects
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // set the primitive topology
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView); // set the vertex buffer (using the vertex buffer view)
    commandList->IASetIndexBuffer(&indexBufferView);

    // first cube

    // set cube1's constant buffer
    commandList->SetGraphicsRootConstantBufferView(0, constantBufferUploadHeaps[frameIndex]->GetGPUVirtualAddress());

    // draw first cube
    commandList->DrawIndexedInstanced(numCubeIndices, 1, 0, 0, 0);

    // second cube

    // set cube2's constant buffer. You can see we are adding the size of ConstantBufferPerObject to the constant buffer
    // resource heaps address. This is because cube1's constant buffer is stored at the beginning of the resource heap, while
    // cube2's constant buffer data is stored after (256 bits from the start of the heap).
    commandList->SetGraphicsRootConstantBufferView(0, constantBufferUploadHeaps[frameIndex]->GetGPUVirtualAddress() + ConstantBufferPerObjectAlignedSize);

    // draw second cube
    commandList->DrawIndexedInstanced(numCubeIndices, 1, 0, 0, 0);

    // transition the "frameIndex" render target from the render target state to the present state. If the debug layer is enabled, you will receive a
    // warning if present is called on the render target when it's not in the present state
    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    ASSERT_SUCCEEDED(commandList->Close());
}

void DXSample::OnInit()
{
    CreateDevice();
    CreateCommandQueue();
    CreateSwapChain();
    CreateFence();
    CreateRT();
    CreateDepthStencil();

    CreateCommandAllocators();
    CreateCommandList();
    CreateVertex();
    CreateConstantBuffer();
    CreateRootSignature();
    CreatePSO();
    CreateViewPort();
    CreateMatrix();
}

void DXSample::OnUpdate()
{
    // update app logic, such as moving the camera or figuring out what objects are in view
    static float rIncrement = 0.00002f;
    static float gIncrement = 0.00006f;
    static float bIncrement = 0.00009f;

    cbColorMultiplierData.colorMultiplier.x += rIncrement;
    cbColorMultiplierData.colorMultiplier.y += gIncrement;
    cbColorMultiplierData.colorMultiplier.z += bIncrement;

    if (cbColorMultiplierData.colorMultiplier.x >= 1.0f || cbColorMultiplierData.colorMultiplier.x <= 0.0f)
    {
        cbColorMultiplierData.colorMultiplier.x = cbColorMultiplierData.colorMultiplier.x >= 1.0f ? 1.0f : 0.0f;
        rIncrement = -rIncrement;
    }
    if (cbColorMultiplierData.colorMultiplier.y >= 1.0f || cbColorMultiplierData.colorMultiplier.y <= 0.0f)
    {
        cbColorMultiplierData.colorMultiplier.y = cbColorMultiplierData.colorMultiplier.y >= 1.0f ? 1.0f : 0.0f;
        gIncrement = -gIncrement;
    }
    if (cbColorMultiplierData.colorMultiplier.z >= 1.0f || cbColorMultiplierData.colorMultiplier.z <= 0.0f)
    {
        cbColorMultiplierData.colorMultiplier.z = cbColorMultiplierData.colorMultiplier.z >= 1.0f ? 1.0f : 0.0f;
        bIncrement = -bIncrement;
    }

    CD3DX12_RANGE readRange(0, 0);    // We do not intend to read from this resource on the CPU. (End is less than or equal to begin)
    ASSERT_SUCCEEDED(constantBufferUploadHeap[frameIndex]->Map(0, &readRange, reinterpret_cast<void**>(&cbColorMultiplierGPUAddress[frameIndex])));
    // copy our ConstantBuffer instance to the mapped constant buffer resource
    memcpy(cbColorMultiplierGPUAddress[frameIndex], &cbColorMultiplierData, sizeof(cbColorMultiplierData));
    constantBufferUploadHeap[frameIndex]->Unmap(0, &readRange);


    // update app logic, such as moving the camera or figuring out what objects are in view

    // map the resource heap to get a gpu virtual address to the beginning of the heap
    ASSERT_SUCCEEDED(constantBufferUploadHeaps[frameIndex]->Map(0, &readRange, reinterpret_cast<void**>(&cbvGPUAddress[frameIndex])));

    // create rotation matrices
    XMMATRIX rotXMat = XMMatrixRotationX(0.0001f);
    XMMATRIX rotYMat = XMMatrixRotationY(0.0002f);
    XMMATRIX rotZMat = XMMatrixRotationZ(0.0003f);

    // add rotation to cube1's rotation matrix and store it
    XMMATRIX rotMat = XMLoadFloat4x4(&cube1RotMat) * rotXMat * rotYMat * rotZMat;
    XMStoreFloat4x4(&cube1RotMat, rotMat);

    // create translation matrix for cube 1 from cube 1's position vector
    XMMATRIX translationMat = XMMatrixTranslationFromVector(XMLoadFloat4(&cube1Position));

    // create cube1's world matrix by first rotating the cube, then positioning the rotated cube
    XMMATRIX worldMat = rotMat * translationMat;

    // store cube1's world matrix
    XMStoreFloat4x4(&cube1WorldMat, worldMat);

    // update constant buffer for cube1
    // create the wvp matrix and store in constant buffer
    XMMATRIX viewMat = XMLoadFloat4x4(&cameraViewMat); // load view matrix
    XMMATRIX projMat = XMLoadFloat4x4(&cameraProjMat); // load projection matrix
    XMMATRIX wvpMat = XMLoadFloat4x4(&cube1WorldMat) * viewMat * projMat; // create wvp matrix
    XMMATRIX transposed = XMMatrixTranspose(wvpMat); // must transpose wvp matrix for the gpu
    XMStoreFloat4x4(&cbPerObject.wvpMat, transposed); // store transposed wvp matrix in constant buffer

   // copy our ConstantBuffer instance to the mapped constant buffer resource
    memcpy(cbvGPUAddress[frameIndex], &cbPerObject, sizeof(cbPerObject));

    // now do cube2's world matrix
    // create rotation matrices for cube2
    rotXMat = XMMatrixRotationX(0.0003f);
    rotYMat = XMMatrixRotationY(0.0002f);
    rotZMat = XMMatrixRotationZ(0.0001f);

    // add rotation to cube2's rotation matrix and store it
    rotMat = rotZMat * (XMLoadFloat4x4(&cube2RotMat) * (rotXMat * rotYMat));
    XMStoreFloat4x4(&cube2RotMat, rotMat);

    // create translation matrix for cube 2 to offset it from cube 1 (its position relative to cube1
    XMMATRIX translationOffsetMat = XMMatrixTranslationFromVector(XMLoadFloat4(&cube2PositionOffset));

    // we want cube 2 to be half the size of cube 1, so we scale it by .5 in all dimensions
    XMMATRIX scaleMat = XMMatrixScaling(0.5f, 0.5f, 0.5f);

    // reuse worldMat.
    // first we scale cube2. scaling happens relative to point 0,0,0, so you will almost always want to scale first
    // then we translate it.
    // then we rotate it. rotation always rotates around point 0,0,0
    // finally we move it to cube 1's position, which will cause it to rotate around cube 1
    worldMat = scaleMat * translationOffsetMat * rotMat * translationMat;

    wvpMat = XMLoadFloat4x4(&cube2WorldMat) * viewMat * projMat; // create wvp matrix
    transposed = XMMatrixTranspose(wvpMat); // must transpose wvp matrix for the gpu
    XMStoreFloat4x4(&cbPerObject.wvpMat, transposed); // store transposed wvp matrix in constant buffer

    // copy our ConstantBuffer instance to the mapped constant buffer resource
    memcpy(cbvGPUAddress[frameIndex] + ConstantBufferPerObjectAlignedSize, &cbPerObject, sizeof(cbPerObject));
    constantBufferUploadHeaps[frameIndex]->Unmap(0, &readRange);

    // store cube2's world matrix
    XMStoreFloat4x4(&cube2WorldMat, worldMat);
}

void DXSample::OnRender()
{
    PopulateCommandList(); // update the pipeline by sending commands to the commandqueue

    // create an array of command lists (only one command list here)
    ID3D12CommandList* ppCommandLists[] = { commandList };

    // execute the array of command lists
    commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // this command goes in at the end of our command queue. we will know when our command queue
    // has finished because the fence value will be set to "fenceValue" from the GPU since the command
    // queue is being executed on the GPU
    ASSERT_SUCCEEDED(commandQueue->Signal(fence[frameIndex], fenceValue[frameIndex]));

    // present the current backbuffer
    ASSERT_SUCCEEDED(swapChain->Present(0, 0));
}

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) if (x != nullptr) { x->Release(); x = nullptr; }
#endif

void DXSample::OnDestroy()
{
    WaitForPreviousFrame();

    // close the fence event
    CloseHandle(fenceEvent);

    // get swapchain out of full screen before exiting
    BOOL fs = false;
    if (swapChain->GetFullscreenState(&fs, NULL))
        swapChain->SetFullscreenState(false, NULL);

    SAFE_RELEASE(device);
    SAFE_RELEASE(swapChain);
    SAFE_RELEASE(commandQueue);
    SAFE_RELEASE(rtvDescriptorHeap);
    SAFE_RELEASE(commandList);

    for (int i = 0; i < frameBufferCount; ++i)
    {
        SAFE_RELEASE(renderTargets[i]);
        SAFE_RELEASE(commandAllocator[i]);
        SAFE_RELEASE(fence[i]);
    }

    SAFE_RELEASE(pipelineStateObject);
    SAFE_RELEASE(rootSignature);
    SAFE_RELEASE(vertexBuffer);
    SAFE_RELEASE(indexBuffer);

    SAFE_RELEASE(depthStencilBuffer);
    SAFE_RELEASE(dsDescriptorHeap);

    for (int i = 0; i < frameBufferCount; ++i)
    {
        SAFE_RELEASE(mainDescriptorHeap[i]);
        SAFE_RELEASE(constantBufferUploadHeap[i]);
    }

    for (int i = 0; i < frameBufferCount; ++i)
    {
        SAFE_RELEASE(constantBufferUploadHeaps[i]);
    }
}

UINT DXSample::GetWidth() const
{
    return m_width;
}

UINT DXSample::GetHeight() const
{
    return m_height;
}