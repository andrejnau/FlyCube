add_library(AppSettings STATIC
    ArgsParser.cpp
    ArgsParser.h
    Settings.h
)

target_include_directories(AppSettings
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(AppSettings FlyCube)

set_target_properties(AppSettings PROPERTIES FOLDER "Modules")
