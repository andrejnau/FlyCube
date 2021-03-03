add_library(dxc INTERFACE)
target_include_directories(dxc INTERFACE "${project_root}/3rdparty/dxc/include")
target_compile_definitions(dxc INTERFACE DXC_CUSTOM_BIN="${project_root}/3rdparty/dxc/bin")
list(APPEND CMAKE_MODULE_PATH "${project_root}/3rdparty/dxc/cmake/modules")
