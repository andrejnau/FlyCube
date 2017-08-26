add_library(ISample INTERFACE)

target_include_directories(ISample
    INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
