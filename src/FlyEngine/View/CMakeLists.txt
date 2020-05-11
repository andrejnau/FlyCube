list(APPEND headers
    View.h
)

if (VULKAN_SUPPORT)
    list(APPEND headers
        VKView.h 
    )
    list(APPEND sources
        VKView.cpp
    )
endif()

if (DIRECTX_SUPPORT)
    list(APPEND headers
        DXView.h
        DXViewCreater.h
        DXViewDescCreater.h
    )
    list(APPEND sources
        DXView.cpp
        DXViewCreater.cpp
        DXViewDescCreater.cpp
    )
endif()

add_library(View ${headers} ${sources})

target_include_directories(View
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(View
    Resource
    Utilities
)

if (VULKAN_SUPPORT)
    target_link_libraries(View
        ${Vulkan_LIBRARIES}
    )
    target_include_directories(View
        PUBLIC
            ${Vulkan_INCLUDE_DIR}
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(View
        d3d11
        d3d12
        dxgi
        dxguid
    )
    target_include_directories(View
        PUBLIC
            "${project_root}/3rdparty/d3dx12"
    )
endif()

set_target_properties(View PROPERTIES FOLDER "FlyEngine")
