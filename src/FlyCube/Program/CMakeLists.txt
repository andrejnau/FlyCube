list(APPEND headers
    Program.h
    ProgramBase.h
)

list(APPEND sources
    ProgramBase.cpp
)

if (VULKAN_SUPPORT)
    list(APPEND headers
        VKProgram.h
    )
    list(APPEND sources
        VKProgram.cpp
    )
endif()

if (DIRECTX_SUPPORT)
    list(APPEND headers
        DXProgram.h
    )
    list(APPEND sources
        DXProgram.cpp
    )
endif()

add_library(Program ${headers} ${sources})

target_include_directories(Program
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(Program
    ApiType
    Device
    Utilities
    Shader
    BindingSet
)

if (VULKAN_SUPPORT)
    target_link_libraries(Program
        vulkan
    )
endif()

if (DIRECTX_SUPPORT)
    target_link_libraries(Program
        d3d12
        dxgi
        dxguid
    )
endif()

set_target_properties(Program PROPERTIES FOLDER "FlyCube")
