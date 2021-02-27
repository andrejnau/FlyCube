include_directories(${project_root}/3rdparty/dxc/include)
set(dxc_custom_bin "${project_root}/3rdparty/dxc/bin")
add_definitions(-DDXC_CUSTOM_BIN="${dxc_custom_bin}")
list(APPEND CMAKE_MODULE_PATH "${project_root}/3rdparty/dxc/cmake/modules")
