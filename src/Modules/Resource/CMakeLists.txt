set(target Resource)

set(headers
    DX11Resource.h
    DX12Resource.h
    GLResource.h
    Resource.h
    VKResource.h
)

set(sources
    DX12Resource.cpp
)

add_library(${target} ${headers} ${sources})

target_link_libraries(${target}
	Context
)

set_target_properties(${target} PROPERTIES FOLDER "library")
