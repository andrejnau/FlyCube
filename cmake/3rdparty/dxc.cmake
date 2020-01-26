include_directories(${project_root}/3rdparty/dxc-artifacts/include)
set(dxc_custom_bin "${project_root}/3rdparty/dxc-artifacts/bin")
add_definitions(-DDXC_CUSTOM_BIN="${dxc_custom_bin}")