set(target View)

set(headers
    View.h
)

set(sources
    View.cpp
)

if (VULKAN_SUPPORT)
    set(headers 
        ${headers}
        VKView.h 
    )
    set(sources 
        ${sources}
        VKView.cpp
    )
endif()

if (DIRECTX_SUPPORT)
    set(headers 
        ${headers}
        DX11View.h
        DX12View.h
    )
    set(sources 
        ${sources}
        DX11View.cpp
        DX12View.cpp
    )
endif()

add_library(${target} ${headers} ${sources})

target_link_libraries(${target}
    Context
)

set_target_properties(${target} PROPERTIES FOLDER "library")
