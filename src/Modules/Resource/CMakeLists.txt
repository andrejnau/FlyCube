set(target Resource)

set(headers
    DX11Resource.h
    DX12Resource.h
    GLResource.h
    Resource.h
)

set(sources
    DX12Resource.cpp
)

if (VULKAN_SUPPORT)
    set(headers 
        ${headers}
        VKResource.h
    )
endif()

add_library(${target} ${headers} ${sources})

target_link_libraries(${target}
    Context
)

set_target_properties(${target} PROPERTIES FOLDER "library")
