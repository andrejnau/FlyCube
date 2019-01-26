set(target View)

set(headers
    DX11View.h
    DX12View.h
    View.h
    VKView.h 
)

set(sources
    DX11View.cpp
    DX12View.cpp
    VKView.cpp
)

add_library(${target} ${headers} ${sources})

target_link_libraries(${target}
    Context
)

set_target_properties(${target} PROPERTIES FOLDER "library")
