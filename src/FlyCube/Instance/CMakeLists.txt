list(APPEND headers
    Instance.h
    BaseTypes.h
)

list(APPEND sources
    Instance.cpp
)

if (VULKAN_SUPPORT)
    list(APPEND headers
        VKInstance.h
    )
    list(APPEND sources
        VKInstance.cpp
    )
endif()

if (DIRECTX_SUPPORT)
    list(APPEND headers
        DXInstance.h
    )
    list(APPEND sources
        DXInstance.cpp
    )
endif()

if (METAL_SUPPORT)
    list(APPEND headers
        MTInstance.h
    )
    list(APPEND sources
        MTInstance.mm
    )
endif()

if (SOFTWARE_SUPPORT)
    list(APPEND headers
        SWInstance.h
    )
    list(APPEND sources
        SWInstance.cpp
    )
endif()

add_library(Instance ${headers} ${sources})

target_include_directories(Instance
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(Instance
    Adapter
    ApiType
    Utilities
    Device
    gli
    glm
    $<$<TARGET_EXISTS:GPUDescriptorPool>:GPUDescriptorPool>
    $<$<TARGET_EXISTS:QueryHeap>:QueryHeap>
)

if (VULKAN_SUPPORT)
    target_link_libraries(Instance
        vulkan
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(Instance
        d3d12
        dxgi
        dxguid
        DirectX-Headers
        DirectX-Guids
    )
endif()

if (METAL_SUPPORT)
    target_link_libraries(Instance
        "-framework Metal"
        "-framework QuartzCore"
    )
    target_compile_options(Instance PUBLIC "-fobjc-arc")
endif()

set_target_properties(Instance PROPERTIES FOLDER "FlyCube")
