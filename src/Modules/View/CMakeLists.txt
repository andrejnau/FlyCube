set(target View)

set(headers
    DX11View.h
    DX12View.h
    View.h
)

set(sources
    DX11View.cpp
    DX12View.cpp
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

add_library(${target} ${headers} ${sources})

target_link_libraries(${target}
    Context
)

set_target_properties(${target} PROPERTIES FOLDER "library")
