list(APPEND headers
    Context.h
    ContextBase.h
    BufferLayout.h
)

list(APPEND sources
    Context.cpp
    ContextBase.cpp
    BufferLayout.cpp
)

add_library(Context ${headers} ${sources})

target_include_directories(Context
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(Context
    RenderCommandList
)

set_target_properties(Context PROPERTIES FOLDER "Modules")
