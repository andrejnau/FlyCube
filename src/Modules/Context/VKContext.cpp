#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#define VK_USE_PLATFORM_WIN32_KHR
#include <GLFW/glfw3.h>
#include <vulkan/vk_sdk_platform.h>
#include "VKContext.h"
#include "VKResource.h"
#include <Program/VKProgramApi.h>
#include <Geometry/IABuffer.h>
#include <glm/glm.hpp>

VKAPI_ATTR VkBool32 VKAPI_CALL MyDebugReportCallback(
    VkDebugReportFlagsEXT       flags,
    VkDebugReportObjectTypeEXT  objectType,
    uint64_t                    object,
    size_t                      location,
    int32_t                     messageCode,
    const char*                 pLayerPrefix,
    const char*                 pMessage,
    void*                       pUserData)
{
    printf("%s\n", pMessage);
    return VK_FALSE;
}

static std::vector<uint8_t> readFile(const char* filename)
{
    // open the file:
    std::streampos fileSize;
    std::ifstream file(filename, std::ios::binary);

    // get its size:
    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // read the data:
    std::vector<unsigned char> fileData(fileSize);
    file.read((char*)&fileData[0], fileSize);
    return fileData;
}

static std::string get_tmp_file(const std::string& prefix)
{
    char tmpdir[MAX_PATH] = {};
    GetTempPathA(MAX_PATH, tmpdir);
    return tmpdir + prefix;
}

static std::vector<uint8_t> hlsl2spirv(ShaderType type, std::string sdpath)
{
    std::string shader_type;
    switch (type)
    {
    case ShaderType::kPixel:
        shader_type = "frag";
        break;
    case ShaderType::kVertex:
        shader_type = "vert";
        break;
    case ShaderType::kGeometry:
        shader_type = "geom";
        break;
    case ShaderType::kCompute:
        shader_type = "comp";
        break;
    }

    std::string spirv_path = get_tmp_file("SponzaApp.spirv");

    std::string cmd = "C:\\VulkanSDK\\1.1.73.0\\Bin\\glslangValidator.exe";
    cmd += " -e ";
    cmd += "main";
    cmd += " -S ";
    cmd += shader_type;
    cmd += " -V ";
    cmd += sdpath;
    cmd += " -o ";
    cmd += spirv_path;

    /*for (auto &x : shader.define)
    {
        cmd += " -D" + x.first + "=" + x.second;
    }*/

    DeleteFileA(spirv_path.c_str());
    system(cmd.c_str());

    auto res = readFile(spirv_path.c_str());

    DeleteFileA(spirv_path.c_str());
    return res;
}

VKContext::VKContext(GLFWwindow* window, int width, int height)
    : Context(window, width, height)
{
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    const char* pInstExt[] = {
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME
    };

    const char* pInstLayers[] = {
        "VK_LAYER_LUNARG_standard_validation"
    };

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = _countof(pInstExt);
    createInfo.ppEnabledExtensionNames = pInstExt;
    createInfo.enabledLayerCount = _countof(pInstLayers);
    createInfo.ppEnabledLayerNames = pInstLayers;

    VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);


    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, NULL);

    assert(layerCount != 0, "Failed to find any layer in your system.");

    std::vector<VkLayerProperties> layersAvailable(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layersAvailable.data());

    bool foundValidation = false;
    for (int i = 0; i < layerCount; ++i) {
        if (strcmp(layersAvailable[i].layerName, "VK_LAYER_LUNARG_standard_validation") == 0) {
            foundValidation = true;
        }
    }

#if 1
    VkDebugReportCallbackCreateInfoEXT callbackCreateInfo;
    callbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    callbackCreateInfo.pNext = NULL;
    callbackCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |
        VK_DEBUG_REPORT_WARNING_BIT_EXT |
        VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
    callbackCreateInfo.pfnCallback = &MyDebugReportCallback;
    callbackCreateInfo.pUserData = NULL;

    PFN_vkCreateDebugReportCallbackEXT my_vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(m_instance, "vkCreateDebugReportCallbackEXT");
    VkDebugReportCallbackEXT callback;
    auto res2 = my_vkCreateDebugReportCallbackEXT(m_instance, &callbackCreateInfo, NULL, &callback);
#endif

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    for (const auto& device : devices)
    {
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
            deviceFeatures.geometryShader)
            physicalDevice = device;
    }

    auto device = physicalDevice;
    m_physical_device = device;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int graphicsFamily = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT &&  queueFamily.queueFlags  & VK_QUEUE_COMPUTE_BIT)
        {
            break;
            int b = 0;
        }
        ++graphicsFamily;
    }
    presentQueueFamily = graphicsFamily;

    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = graphicsFamily;
    queueCreateInfo.queueCount = 1;
    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures = {};

    VkDeviceCreateInfo createInfo2 = {};
    createInfo2.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo2.pQueueCreateInfos = &queueCreateInfo;
    createInfo2.queueCreateInfoCount = 1;
    createInfo2.pEnabledFeatures = &deviceFeatures;

    const char* pDevExt[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    createInfo2.enabledExtensionCount = _countof(pDevExt);
    createInfo2.ppEnabledExtensionNames = pDevExt;

    if (vkCreateDevice(physicalDevice, &createInfo2, nullptr, &m_device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }
    vkGetDeviceQueue(m_device, graphicsFamily, 0, &m_queue);

    auto ptr = vkCreateSwapchainKHR;

    glfwCreateWindowSurface(m_instance, window, nullptr, &m_surface);

    {
        VkBool32 is = false;
        auto resdfg = vkGetPhysicalDeviceSurfaceSupportKHR(device, graphicsFamily, m_surface, &is);
        int b = 0;
    }

    VkExtent2D surfaceResolution;
    {
        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface,
            &formatCount, NULL);
        std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface,
            &formatCount, surfaceFormats.data());

        // If the format list includes just one entry of VK_FORMAT_UNDEFINED, the surface has
        // no preferred format. Otherwise, at least one supported format will be returned.
        VkFormat colorFormat;
        if (formatCount == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED) {
            colorFormat = VK_FORMAT_B8G8R8_UNORM;
        }
        else {
            colorFormat = surfaceFormats[0].format;
        }
        VkColorSpaceKHR colorSpace;
        colorSpace = surfaceFormats[0].colorSpace;


        VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface,
            &surfaceCapabilities);

        // we are effectively looking for double-buffering:
        // if surfaceCapabilities.maxImageCount == 0 there is actually no limit on the number of images! 
        uint32_t desiredImageCount = 2;
        if (desiredImageCount < surfaceCapabilities.minImageCount) {
            desiredImageCount = surfaceCapabilities.minImageCount;
        }
        else if (surfaceCapabilities.maxImageCount != 0 &&
            desiredImageCount > surfaceCapabilities.maxImageCount) {
            desiredImageCount = surfaceCapabilities.maxImageCount;
        }

        surfaceResolution = surfaceCapabilities.currentExtent;
        if (surfaceResolution.width == -1) {
            surfaceResolution.width = width;
            surfaceResolution.height = height;
        }
        else {
            width = surfaceResolution.width;
            height = surfaceResolution.height;
        }

        VkSurfaceTransformFlagBitsKHR preTransform = surfaceCapabilities.currentTransform;
        if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
            preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        }



        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface,
            &presentModeCount, NULL);
        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface,
            &presentModeCount, presentModes.data());

        VkPresentModeKHR presentationMode = VK_PRESENT_MODE_FIFO_KHR;   // always supported.
        for (uint32_t i = 0; i < presentModeCount; ++i) {
            if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
                presentationMode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
        }
    }




    VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface = m_surface;
    swapChainCreateInfo.minImageCount = FrameCount;
    swapChainCreateInfo.imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
    swapChainCreateInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapChainCreateInfo.imageExtent = surfaceResolution;
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapChainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapChainCreateInfo.clipped = true;

    VkResult res = vkCreateSwapchainKHR(m_device, &swapChainCreateInfo, nullptr, &m_swapchain);

    uint32_t NumSwapChainImages = 0;
    res = vkGetSwapchainImagesKHR(m_device, m_swapchain, &NumSwapChainImages, nullptr);

    m_images.resize(NumSwapChainImages);
    m_cmd_bufs.resize(NumSwapChainImages);
    res = vkGetSwapchainImagesKHR(m_device, m_swapchain, &NumSwapChainImages, m_images.data());

    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCreateInfo.queueFamilyIndex = graphicsFamily;
    res = vkCreateCommandPool(m_device, &commandPoolCreateInfo, nullptr, &m_cmd_pool);


    VkCommandBufferAllocateInfo cmdBufAllocInfo = {};
    cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufAllocInfo.commandPool = m_cmd_pool;
    cmdBufAllocInfo.commandBufferCount = m_cmd_bufs.size();
    cmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    res = vkAllocateCommandBuffers(m_device, &cmdBufAllocInfo, m_cmd_bufs.data());

    m_image_views.resize(m_images.size());
    for (size_t i = 0; i < m_images.size(); ++i)
    {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        VkResult res = vkBeginCommandBuffer(m_cmd_bufs[i], &beginInfo);

        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_images[i];
        createInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(m_device, &createInfo, nullptr, &m_image_views[i]);
    }

    int b = 0;


    VkSemaphoreCreateInfo createInfoSemaphore = {};
    createInfoSemaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    vkCreateSemaphore(m_device, &createInfoSemaphore, nullptr, &imageAvailableSemaphore);
    vkCreateSemaphore(m_device, &createInfoSemaphore, nullptr, &renderingFinishedSemaphore);



    std::vector<glm::vec2> position =
    {
        { 0.0f, -0.5f },
        { 0.5f, 0.5f },
        { -0.5f, 0.5f }
    };

    std::vector<glm::vec3> colors = {
        { 1.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f }
    };

    m_positions_buffer.reset(new IAVertexBuffer(*this, position));
    m_colors_buffer.reset(new IAVertexBuffer(*this, colors));

    const std::vector<uint16_t> indices = {
        0, 1, 2,
    };

    m_indices_buffer.reset(new IAIndexBuffer(*this, indices, DXGI_FORMAT_R16_UINT));

    VkVertexInputBindingDescription bindingDescription[2] = {};
    bindingDescription[0].binding = 0;
    bindingDescription[0].stride = sizeof(glm::vec2);
    bindingDescription[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    bindingDescription[1].binding = 1;
    bindingDescription[1].stride = sizeof(glm::vec3);
    bindingDescription[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;


    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = 0;

    attributeDescriptions[1].binding = 1;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = 0;


    swapChainExtent = { 1u*width ,1u*height };


    VkPipelineLayout pipelineLayout;


    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    vertexInputInfo.vertexBindingDescriptionCount = 2;
    vertexInputInfo.pVertexBindingDescriptions = bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapChainExtent.width;
    viewport.height = (float)swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    res = vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &pipelineLayout);


    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;


    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subPass = {};
    subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    subPass.colorAttachmentCount = 1;
    subPass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subPass;

    res = vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &renderPass);

    VkShaderModule vertShaderModule;
    {  
        std::vector<uint8_t> code = hlsl2spirv(ShaderType::kVertex, GetAssetFullPath("\\shaders\\VulkanDemo\\vert.glsl"));
        VkShaderModuleCreateInfo createInfoS = {};
        createInfoS.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfoS.codeSize = code.size();
        createInfoS.pCode = (uint32_t*)code.data();
        vkCreateShaderModule(m_device, &createInfoS, nullptr, &vertShaderModule);
    }

    VkShaderModule fragShaderModule;
    {
        std::vector<uint8_t> code = hlsl2spirv(ShaderType::kPixel, GetAssetFullPath("\\shaders\\VulkanDemo\\frag.glsl"));
        VkShaderModuleCreateInfo createInfoS = {};
        createInfoS.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfoS.codeSize = code.size();
        createInfoS.pCode = (uint32_t*)code.data();
        vkCreateShaderModule(m_device, &createInfoS, nullptr, &fragShaderModule);
    }

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
    

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;

    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr; // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr; // Optional

    pipelineInfo.layout = pipelineLayout;

    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1; // Optional


    res = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline);



    swapChainFramebuffers.resize(m_image_views.size());

    for (size_t i = 0; i < m_image_views.size(); i++) {
        VkImageView attachments[] = {
            m_image_views[i]
        };

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }

    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkCreateFence(m_device, &fenceCreateInfo, NULL, &renderFence);

    int bb = 0;
}

std::unique_ptr<ProgramApi> VKContext::CreateProgram()
{
    return std::make_unique<VKProgramApi>(*this);
}

Resource::Ptr VKContext::CreateTexture(uint32_t bind_flag, DXGI_FORMAT format, uint32_t msaa_count, int width, int height, int depth, int mip_levels)
{
    return Resource::Ptr();
}

uint32_t VKContext::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physical_device, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}

Resource::Ptr VKContext::CreateBuffer(uint32_t bind_flag, uint32_t buffer_size, uint32_t stride)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = buffer_size;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (bind_flag & BindFlag::kVbv)
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    else if(bind_flag & BindFlag::kIbv)
        bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    else
        return Resource::Ptr();

    VKResource::Ptr res = std::make_shared<VKResource>();

    vkCreateBuffer(m_device, &bufferInfo, nullptr, &res->buf);

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, res->buf, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);


    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &res->bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate vertex buffer memory!");
    }

    vkBindBufferMemory(m_device, res->buf, res->bufferMemory, 0);
    res->buffer_size = buffer_size;

    return res;
}

void VKContext::UpdateSubresource(const Resource::Ptr & ires, uint32_t DstSubresource, const void * pSrcData, uint32_t SrcRowPitch, uint32_t SrcDepthPitch)
{
    if (!ires)
        return;
    auto res = std::static_pointer_cast<VKResource>(ires);

    if (!res->bufferMemory)
        return;

    void* data;
    vkMapMemory(m_device, res->bufferMemory, 0, res->buffer_size, 0, &data);
    memcpy(data, pSrcData, (size_t)res->buffer_size);
    vkUnmapMemory(m_device, res->bufferMemory);
}

void VKContext::SetViewport(float width, float height)
{
}

void VKContext::SetScissorRect(int32_t left, int32_t top, int32_t right, int32_t bottom)
{
}

void VKContext::IASetIndexBuffer(Resource::Ptr ires, uint32_t SizeInBytes, DXGI_FORMAT Format)
{
    VKResource::Ptr res = std::static_pointer_cast<VKResource>(ires);
    vkCmdBindIndexBuffer(m_cmd_bufs[m_frame_index], res->buf, 0, VK_INDEX_TYPE_UINT16);
}

void VKContext::IASetVertexBuffer(uint32_t slot, Resource::Ptr ires, uint32_t SizeInBytes, uint32_t Stride)
{
    VKResource::Ptr res = std::static_pointer_cast<VKResource>(ires);
    VkBuffer vertexBuffers[] = { res->buf };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(m_cmd_bufs[m_frame_index], slot, 1, vertexBuffers, offsets);
}

void VKContext::BeginEvent(LPCWSTR Name)
{
}

void VKContext::EndEvent()
{
}

void VKContext::DrawIndexed(uint32_t IndexCount, uint32_t StartIndexLocation, int32_t BaseVertexLocation)
{
}

void VKContext::Dispatch(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ)
{
}

Resource::Ptr VKContext::GetBackBuffer()
{
    return Resource::Ptr();
}

void VKContext::Present(const Resource::Ptr & ires)
{
  

     vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, imageAvailableSemaphore, nullptr, &m_frame_index);



  


    VkClearColorValue clearColor = { 164.0f / 256.0f, 30.0f / 256.0f, 34.0f / 256.0f, 0.0f };
    VkClearValue clearValue = {};
    clearValue.color = clearColor;

    VkImageSubresourceRange imageRange = {};
    imageRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageRange.levelCount = 1;
    imageRange.layerCount = 1;


    VkImageMemoryBarrier presentToClearBarrier = {};
    presentToClearBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    presentToClearBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    presentToClearBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    presentToClearBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    presentToClearBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    presentToClearBarrier.srcQueueFamilyIndex = presentQueueFamily;
    presentToClearBarrier.dstQueueFamilyIndex = presentQueueFamily;
    presentToClearBarrier.image = m_images[m_frame_index];
    presentToClearBarrier.subresourceRange = imageRange;

    // Change layout of image to be optimal for presenting
    VkImageMemoryBarrier clearToPresentBarrier = {};
    clearToPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    clearToPresentBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    clearToPresentBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    clearToPresentBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    clearToPresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    clearToPresentBarrier.srcQueueFamilyIndex = presentQueueFamily;
    clearToPresentBarrier.dstQueueFamilyIndex = presentQueueFamily;
    clearToPresentBarrier.image = m_images[m_frame_index];
    clearToPresentBarrier.subresourceRange = imageRange;


    vkCmdPipelineBarrier(m_cmd_bufs[m_frame_index], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &presentToClearBarrier);

    vkCmdClearColorImage(m_cmd_bufs[m_frame_index], m_images[m_frame_index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &imageRange);


    vkCmdPipelineBarrier(m_cmd_bufs[m_frame_index], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &clearToPresentBarrier);



    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[m_frame_index];

    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapChainExtent;

    VkClearValue clearColor2 = { 0.0f, 0.0f, 0.0f, 1.0f };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor2;

    vkCmdBeginRenderPass(m_cmd_bufs[m_frame_index], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(m_cmd_bufs[m_frame_index], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);


    m_positions_buffer->BindToSlot(0);
    m_colors_buffer->BindToSlot(1);
    m_indices_buffer->Bind();

    vkCmdDrawIndexed(m_cmd_bufs[m_frame_index], m_indices_buffer->Count(), 1, 0, 0, 0);

    vkCmdEndRenderPass(m_cmd_bufs[m_frame_index]);




    auto res = vkEndCommandBuffer(m_cmd_bufs[m_frame_index]);





    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_cmd_bufs[m_frame_index];
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
    VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    submitInfo.pWaitDstStageMask = &waitDstStageMask;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderingFinishedSemaphore;

     res = vkQueueSubmit(m_queue, 1, &submitInfo, renderFence);

   


    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_swapchain;
    presentInfo.pImageIndices = &m_frame_index;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderingFinishedSemaphore;

    res = vkQueuePresentKHR(m_queue, &presentInfo);


    vkWaitForFences(m_device, 1, &renderFence, VK_TRUE, UINT64_MAX);
    vkResetFences(m_device, 1, &renderFence);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    res = vkBeginCommandBuffer(m_cmd_bufs[m_frame_index], &beginInfo);

 
}

void VKContext::ResizeBackBuffer(int width, int height)
{
}
