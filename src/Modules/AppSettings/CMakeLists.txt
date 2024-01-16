list(APPEND headers
    ArgsParser.h
    Settings.h
)

list(APPEND sources
    ArgsParser.cpp
)

add_library(AppSettings ${headers} ${sources})

target_include_directories(AppSettings
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(AppSettings FlyCube)

set_target_properties(AppSettings PROPERTIES FOLDER "Modules")
