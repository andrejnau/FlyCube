list(APPEND headers
    Context.h
)

list(APPEND sources
    Context.cpp
)

add_library(Context ${headers} ${sources})

target_include_directories(Context
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(Context
    CommandListBox
)

set_target_properties(Context PROPERTIES FOLDER "Modules")
