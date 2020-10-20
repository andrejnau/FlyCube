list(APPEND headers
    RenderDevice.h
    RenderDeviceImpl.h
)

list(APPEND sources
    RenderDeviceImpl.cpp
)

add_library(RenderDevice ${headers} ${sources})

target_include_directories(RenderDevice
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(RenderDevice
    RenderCommandList
)

set_target_properties(RenderDevice PROPERTIES FOLDER "Modules")
