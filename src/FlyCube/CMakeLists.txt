list(APPEND Adapter
    $<$<BOOL:${DIRECTX_SUPPORT}>:Adapter/DXAdapter.cpp>
    $<$<BOOL:${DIRECTX_SUPPORT}>:Adapter/DXAdapter.h>
    $<$<BOOL:${METAL_SUPPORT}>:Adapter/MTAdapter.h>
    $<$<BOOL:${METAL_SUPPORT}>:Adapter/MTAdapter.mm>
    $<$<BOOL:${VULKAN_SUPPORT}>:Adapter/VKAdapter.cpp>
    $<$<BOOL:${VULKAN_SUPPORT}>:Adapter/VKAdapter.h>
    Adapter/Adapter.h
)

list(APPEND ApiType
    ApiType/ApiType.h
)

list(APPEND BindingSet
    $<$<BOOL:${DIRECTX_SUPPORT}>:BindingSet/DXBindingSet.cpp>
    $<$<BOOL:${DIRECTX_SUPPORT}>:BindingSet/DXBindingSet.h>
    $<$<BOOL:${METAL_SUPPORT}>:BindingSet/MTBindingSet.h>
    $<$<BOOL:${METAL_SUPPORT}>:BindingSet/MTBindingSet.mm>
    $<$<BOOL:${VULKAN_SUPPORT}>:BindingSet/VKBindingSet.cpp>
    $<$<BOOL:${VULKAN_SUPPORT}>:BindingSet/VKBindingSet.h>
    BindingSet/BindingSet.h
)

list(APPEND BindingSetLayout
    $<$<BOOL:${DIRECTX_SUPPORT}>:BindingSetLayout/DXBindingSetLayout.cpp>
    $<$<BOOL:${DIRECTX_SUPPORT}>:BindingSetLayout/DXBindingSetLayout.h>
    $<$<BOOL:${METAL_SUPPORT}>:BindingSetLayout/MTBindingSetLayout.h>
    $<$<BOOL:${METAL_SUPPORT}>:BindingSetLayout/MTBindingSetLayout.mm>
    $<$<BOOL:${VULKAN_SUPPORT}>:BindingSetLayout/VKBindingSetLayout.cpp>
    $<$<BOOL:${VULKAN_SUPPORT}>:BindingSetLayout/VKBindingSetLayout.h>
    BindingSetLayout/BindingSetLayout.h
)

list(APPEND CPUDescriptorPool
    $<$<BOOL:${DIRECTX_SUPPORT}>:CPUDescriptorPool/DXCPUDescriptorHandle.cpp>
    $<$<BOOL:${DIRECTX_SUPPORT}>:CPUDescriptorPool/DXCPUDescriptorHandle.h>
    $<$<BOOL:${DIRECTX_SUPPORT}>:CPUDescriptorPool/DXCPUDescriptorPool.cpp>
    $<$<BOOL:${DIRECTX_SUPPORT}>:CPUDescriptorPool/DXCPUDescriptorPool.h>
    $<$<BOOL:${DIRECTX_SUPPORT}>:CPUDescriptorPool/DXCPUDescriptorPoolTyped.cpp>
    $<$<BOOL:${DIRECTX_SUPPORT}>:CPUDescriptorPool/DXCPUDescriptorPoolTyped.h>
)

list(APPEND CommandList
    $<$<BOOL:${DIRECTX_SUPPORT}>:CommandList/DXCommandList.cpp>
    $<$<BOOL:${DIRECTX_SUPPORT}>:CommandList/DXCommandList.h>
    $<$<BOOL:${METAL_SUPPORT}>:CommandList/MTCommandList.h>
    $<$<BOOL:${METAL_SUPPORT}>:CommandList/MTCommandList.mm>
    $<$<BOOL:${VULKAN_SUPPORT}>:CommandList/VKCommandList.cpp>
    $<$<BOOL:${VULKAN_SUPPORT}>:CommandList/VKCommandList.h>
    CommandList/CommandList.h
    CommandList/RecordCommandList.h
)

list(APPEND CommandQueue
    $<$<BOOL:${DIRECTX_SUPPORT}>:CommandQueue/DXCommandQueue.cpp>
    $<$<BOOL:${DIRECTX_SUPPORT}>:CommandQueue/DXCommandQueue.h>
    $<$<BOOL:${METAL_SUPPORT}>:CommandQueue/MTCommandQueue.h>
    $<$<BOOL:${METAL_SUPPORT}>:CommandQueue/MTCommandQueue.mm>
    $<$<BOOL:${VULKAN_SUPPORT}>:CommandQueue/VKCommandQueue.cpp>
    $<$<BOOL:${VULKAN_SUPPORT}>:CommandQueue/VKCommandQueue.h>
    CommandQueue/CommandQueue.h
)

list(APPEND Device
    $<$<BOOL:${DIRECTX_SUPPORT}>:Device/DXDevice.cpp>
    $<$<BOOL:${DIRECTX_SUPPORT}>:Device/DXDevice.h>
    $<$<BOOL:${METAL_SUPPORT}>:Device/MTDevice.h>
    $<$<BOOL:${METAL_SUPPORT}>:Device/MTDevice.mm>
    $<$<BOOL:${VULKAN_SUPPORT}>:Device/VKDevice.cpp>
    $<$<BOOL:${VULKAN_SUPPORT}>:Device/VKDevice.h>
    Device/Device.h
)

list(APPEND Fence
    $<$<BOOL:${DIRECTX_SUPPORT}>:Fence/DXFence.cpp>
    $<$<BOOL:${DIRECTX_SUPPORT}>:Fence/DXFence.h>
    $<$<BOOL:${METAL_SUPPORT}>:Fence/MTFence.h>
    $<$<BOOL:${METAL_SUPPORT}>:Fence/MTFence.mm>
    $<$<BOOL:${VULKAN_SUPPORT}>:Fence/VKTimelineSemaphore.cpp>
    $<$<BOOL:${VULKAN_SUPPORT}>:Fence/VKTimelineSemaphore.h>
    Fence/Fence.h
)

list(APPEND GPUDescriptorPool
    $<$<BOOL:${DIRECTX_SUPPORT}>:GPUDescriptorPool/DXGPUDescriptorPool.cpp>
    $<$<BOOL:${DIRECTX_SUPPORT}>:GPUDescriptorPool/DXGPUDescriptorPool.h>
    $<$<BOOL:${DIRECTX_SUPPORT}>:GPUDescriptorPool/DXGPUDescriptorPoolRange.cpp>
    $<$<BOOL:${DIRECTX_SUPPORT}>:GPUDescriptorPool/DXGPUDescriptorPoolRange.h>
    $<$<BOOL:${DIRECTX_SUPPORT}>:GPUDescriptorPool/DXGPUDescriptorPoolTyped.cpp>
    $<$<BOOL:${DIRECTX_SUPPORT}>:GPUDescriptorPool/DXGPUDescriptorPoolTyped.h>
    $<$<BOOL:${METAL_SUPPORT}>:GPUDescriptorPool/MTGPUArgumentBufferRange.h>
    $<$<BOOL:${METAL_SUPPORT}>:GPUDescriptorPool/MTGPUArgumentBufferRange.mm>
    $<$<BOOL:${METAL_SUPPORT}>:GPUDescriptorPool/MTGPUBindlessArgumentBuffer.h>
    $<$<BOOL:${METAL_SUPPORT}>:GPUDescriptorPool/MTGPUBindlessArgumentBuffer.mm>
    $<$<BOOL:${VULKAN_SUPPORT}>:GPUDescriptorPool/VKGPUBindlessDescriptorPoolTyped.cpp>
    $<$<BOOL:${VULKAN_SUPPORT}>:GPUDescriptorPool/VKGPUBindlessDescriptorPoolTyped.h>
    $<$<BOOL:${VULKAN_SUPPORT}>:GPUDescriptorPool/VKGPUDescriptorPool.cpp>
    $<$<BOOL:${VULKAN_SUPPORT}>:GPUDescriptorPool/VKGPUDescriptorPool.h>
    $<$<BOOL:${VULKAN_SUPPORT}>:GPUDescriptorPool/VKGPUDescriptorPoolRange.cpp>
    $<$<BOOL:${VULKAN_SUPPORT}>:GPUDescriptorPool/VKGPUDescriptorPoolRange.h>
)

list(APPEND HLSLCompiler
    HLSLCompiler/Compiler.h
    HLSLCompiler/Compiler.cpp
    HLSLCompiler/DXCLoader.cpp
    HLSLCompiler/DXCLoader.h
    HLSLCompiler/MSLConverter.cpp
    HLSLCompiler/MSLConverter.h
)

list(APPEND Instance
    $<$<BOOL:${DIRECTX_SUPPORT}>:Instance/DXInstance.cpp>
    $<$<BOOL:${DIRECTX_SUPPORT}>:Instance/DXInstance.h>
    $<$<BOOL:${METAL_SUPPORT}>:Instance/MTInstance.h>
    $<$<BOOL:${METAL_SUPPORT}>:Instance/MTInstance.mm>
    $<$<BOOL:${STATIC_MOLTEN_VK}>:Instance/MoltenVKStubs.cpp>
    $<$<BOOL:${VULKAN_SUPPORT}>:Instance/VKInstance.cpp>
    $<$<BOOL:${VULKAN_SUPPORT}>:Instance/VKInstance.h>
    Instance/BaseTypes.h
    Instance/EnumUtils.h
    Instance/Instance.cpp
    Instance/Instance.h
    Instance/QueryInterface.h
)

list(APPEND Memory
    $<$<BOOL:${DIRECTX_SUPPORT}>:Memory/DXMemory.cpp>
    $<$<BOOL:${DIRECTX_SUPPORT}>:Memory/DXMemory.h>
    $<$<BOOL:${METAL_SUPPORT}>:Memory/MTMemory.h>
    $<$<BOOL:${METAL_SUPPORT}>:Memory/MTMemory.mm>
    $<$<BOOL:${VULKAN_SUPPORT}>:Memory/VKMemory.cpp>
    $<$<BOOL:${VULKAN_SUPPORT}>:Memory/VKMemory.h>
    Memory/Memory.h
)

list(APPEND Pipeline
    $<$<BOOL:${DIRECTX_SUPPORT}>:Pipeline/DXComputePipeline.cpp>
    $<$<BOOL:${DIRECTX_SUPPORT}>:Pipeline/DXComputePipeline.h>
    $<$<BOOL:${DIRECTX_SUPPORT}>:Pipeline/DXGraphicsPipeline.cpp>
    $<$<BOOL:${DIRECTX_SUPPORT}>:Pipeline/DXGraphicsPipeline.h>
    $<$<BOOL:${DIRECTX_SUPPORT}>:Pipeline/DXPipeline.cpp>
    $<$<BOOL:${DIRECTX_SUPPORT}>:Pipeline/DXPipeline.h>
    $<$<BOOL:${DIRECTX_SUPPORT}>:Pipeline/DXRayTracingPipeline.cpp>
    $<$<BOOL:${DIRECTX_SUPPORT}>:Pipeline/DXRayTracingPipeline.h>
    $<$<BOOL:${DIRECTX_SUPPORT}>:Pipeline/DXStateBuilder.h>
    $<$<BOOL:${METAL_SUPPORT}>:Pipeline/MTComputePipeline.h>
    $<$<BOOL:${METAL_SUPPORT}>:Pipeline/MTComputePipeline.mm>
    $<$<BOOL:${METAL_SUPPORT}>:Pipeline/MTGraphicsPipeline.h>
    $<$<BOOL:${METAL_SUPPORT}>:Pipeline/MTGraphicsPipeline.mm>
    $<$<BOOL:${METAL_SUPPORT}>:Pipeline/MTPipeline.h>
    $<$<BOOL:${VULKAN_SUPPORT}>:Pipeline/VKComputePipeline.cpp>
    $<$<BOOL:${VULKAN_SUPPORT}>:Pipeline/VKComputePipeline.h>
    $<$<BOOL:${VULKAN_SUPPORT}>:Pipeline/VKGraphicsPipeline.cpp>
    $<$<BOOL:${VULKAN_SUPPORT}>:Pipeline/VKGraphicsPipeline.h>
    $<$<BOOL:${VULKAN_SUPPORT}>:Pipeline/VKPipeline.cpp>
    $<$<BOOL:${VULKAN_SUPPORT}>:Pipeline/VKPipeline.h>
    $<$<BOOL:${VULKAN_SUPPORT}>:Pipeline/VKRayTracingPipeline.cpp>
    $<$<BOOL:${VULKAN_SUPPORT}>:Pipeline/VKRayTracingPipeline.h>
    Pipeline/Pipeline.h
)

list(APPEND Program
    Program/Program.h
    Program/ProgramBase.cpp
    Program/ProgramBase.h
)

list(APPEND QueryHeap
    $<$<BOOL:${DIRECTX_SUPPORT}>:QueryHeap/DXRayTracingQueryHeap.cpp>
    $<$<BOOL:${DIRECTX_SUPPORT}>:QueryHeap/DXRayTracingQueryHeap.h>
    $<$<BOOL:${VULKAN_SUPPORT}>:QueryHeap/VKQueryHeap.cpp>
    $<$<BOOL:${VULKAN_SUPPORT}>:QueryHeap/VKQueryHeap.h>
    QueryHeap/QueryHeap.h
)

list(APPEND Resource
    $<$<BOOL:${DIRECTX_SUPPORT}>:Resource/DXResource.cpp>
    $<$<BOOL:${DIRECTX_SUPPORT}>:Resource/DXResource.h>
    $<$<BOOL:${METAL_SUPPORT}>:Resource/MTResource.h>
    $<$<BOOL:${METAL_SUPPORT}>:Resource/MTResource.mm>
    $<$<BOOL:${VULKAN_SUPPORT}>:Resource/VKResource.cpp>
    $<$<BOOL:${VULKAN_SUPPORT}>:Resource/VKResource.h>
    Resource/Resource.h
    Resource/ResourceBase.cpp
    Resource/ResourceBase.h
)

list(APPEND Shader
    $<$<BOOL:${METAL_SUPPORT}>:Shader/MTShader.h>
    $<$<BOOL:${METAL_SUPPORT}>:Shader/MTShader.mm>
    Shader/Shader.h
    Shader/ShaderBase.cpp
    Shader/ShaderBase.h
)

list(APPEND ShaderReflection
    ShaderReflection/DXILReflection.cpp
    ShaderReflection/DXILReflection.h
    ShaderReflection/DXReflection.cpp
    ShaderReflection/DXReflection.h
    ShaderReflection/ShaderReflection.cpp
    ShaderReflection/ShaderReflection.h
    ShaderReflection/SPIRVReflection.cpp
    ShaderReflection/SPIRVReflection.h
)

list(APPEND Swapchain
    $<$<BOOL:${DIRECTX_SUPPORT}>:Swapchain/DXSwapchain.cpp>
    $<$<BOOL:${DIRECTX_SUPPORT}>:Swapchain/DXSwapchain.h>
    $<$<BOOL:${METAL_SUPPORT}>:Swapchain/MTSwapchain.h>
    $<$<BOOL:${METAL_SUPPORT}>:Swapchain/MTSwapchain.mm>
    $<$<BOOL:${VULKAN_SUPPORT}>:Swapchain/VKSwapchain.cpp>
    $<$<BOOL:${VULKAN_SUPPORT}>:Swapchain/VKSwapchain.h>
    Swapchain/Swapchain.h
)

if (APPLE AND VULKAN_SUPPORT)
    set_property(SOURCE Swapchain/VKSwapchain.cpp PROPERTY COMPILE_FLAGS "-xobjective-c++")
endif()

list(APPEND Utilities
    Utilities/Asset.cpp
    Utilities/Asset.h
    Utilities/Check.h
    Utilities/Common.cpp
    Utilities/Common.h
    Utilities/DXGIFormatHelper.cpp
    Utilities/DXGIFormatHelper.h
    Utilities/DXUtility.h
    Utilities/FormatHelper.cpp
    Utilities/FormatHelper.h
    Utilities/Logging.cpp
    Utilities/Logging.h
    Utilities/NotReached.h
    Utilities/ObjcFormatter.h
    Utilities/PassKey.h
    Utilities/ScopeGuard.h
    Utilities/SystemUtils.cpp
    Utilities/SystemUtils.h
    Utilities/VKUtility.h
)

if (APPLE)
    set_property(SOURCE Utilities/Asset.cpp PROPERTY COMPILE_FLAGS "-xobjective-c++")
endif()

list(APPEND View
    $<$<BOOL:${DIRECTX_SUPPORT}>:View/DXView.cpp>
    $<$<BOOL:${DIRECTX_SUPPORT}>:View/DXView.h>
    $<$<BOOL:${METAL_SUPPORT}>:View/MTView.h>
    $<$<BOOL:${METAL_SUPPORT}>:View/MTView.mm>
    $<$<BOOL:${VULKAN_SUPPORT}>:View/VKView.cpp>
    $<$<BOOL:${VULKAN_SUPPORT}>:View/VKView.h>
    View/View.h
)

add_library(FlyCube
    ${Adapter}
    ${ApiType}
    ${BindingSet}
    ${BindingSetLayout}
    ${CommandList}
    ${CommandQueue}
    ${CPUDescriptorPool}
    ${Device}
    ${Fence}
    ${GPUDescriptorPool}
    ${HLSLCompiler}
    ${Instance}
    ${Memory}
    ${Pipeline}
    ${Program}
    ${QueryHeap}
    ${Resource}
    ${Shader}
    ${ShaderReflection}
    ${Swapchain}
    ${Utilities}
    ${View}
)

target_link_libraries(FlyCube
    $<$<BOOL:${APPLE}>:-framework\ Foundation>
    $<$<BOOL:${APPLE}>:-framework\ QuartzCore>
    $<$<BOOL:${DIRECTX_SUPPORT}>:d3d12>
    $<$<BOOL:${DIRECTX_SUPPORT}>:dxgi>
    $<$<BOOL:${DIRECTX_SUPPORT}>:dxguid>
    $<$<BOOL:${METAL_SUPPORT}>:-framework\ Metal>
    $<$<BOOL:${METAL_SUPPORT}>:MVKPixelFormats>
    $<$<BOOL:${VULKAN_SUPPORT}>:vulkan>
    DirectX-Headers
    dxc
    gli
    glm
    nowide
    spirv-cross-core
    spirv-cross-hlsl
    spirv-cross-msl
)

if (NOT WIN32 AND NOT APPLE AND NOT ANDROID)
    target_link_libraries(FlyCube X11 X11-xcb)
endif()

target_include_directories(FlyCube
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

if (APPLE)
    target_compile_options(FlyCube PUBLIC
        "$<$<COMPILE_LANGUAGE:CXX>:-fobjc-arc>"
    )
endif()

file(GLOB folder_items LIST_DIRECTORIES true "*")
foreach(item ${folder_items})
    if (IS_DIRECTORY ${item})
        cmake_path(GET item FILENAME dir_name)
        source_group("${dir_name}" REGULAR_EXPRESSION "${dir_name}/*")
    endif()
endforeach()

if (BUILD_TESTING)
    add_subdirectory(HLSLCompiler/test)
    add_subdirectory(ShaderReflection/test)
endif()
