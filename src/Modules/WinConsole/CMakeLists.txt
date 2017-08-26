add_library(WinConsole INTERFACE)

target_include_directories(WinConsole
    INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
