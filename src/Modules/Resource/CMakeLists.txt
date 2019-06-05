set(target Resource)

set(headers
    GLResource.h
    Resource.h
)

set(sources
    Resource.cpp
)

if (VULKAN_SUPPORT)
    set(headers 
        ${headers}
        VKResource.h
    )
endif()

if (DIRECTX_SUPPORT)
    set(headers 
        ${headers}
        DX11Resource.h
        DX12Resource.h
    )
    set(sources 
        ${sources}
        DX12Resource.cpp
    )
endif()

add_library(${target} ${headers} ${sources})

target_link_libraries(${target}
    Context
)

set_target_properties(${target} PROPERTIES FOLDER "library")
