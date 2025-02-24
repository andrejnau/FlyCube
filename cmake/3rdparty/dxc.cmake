add_library(dxc INTERFACE)

if (APPLE)
    set(dxc_include_dir "${project_root}/3rdparty/dxc/include_macos")
else()
    set(dxc_include_dir "${project_root}/3rdparty/dxc/include")
endif()

target_include_directories(dxc INTERFACE "${dxc_include_dir}")
target_compile_definitions(dxc INTERFACE DXC_CUSTOM_LOCATION="${project_root}/3rdparty/dxc/bin")

set(get_include "${PROJECT_BINARY_DIR}/gen/include")
configure_file(
    "${dxc_include_dir}/dxc/config.h.cmake"
    "${get_include}/dxc/config.h"
)
target_include_directories(dxc INTERFACE "${get_include}")

if (NOT WIN32)
    target_compile_definitions(dxc INTERFACE __EMULATE_UUID)
endif()

if (CMAKE_SIZEOF_VOID_P STREQUAL "4" AND NOT CMAKE_CROSSCOMPILING)
    message(FATAL_ERROR "x86 build is not supported due to lack of dxc binaries for this platform")
endif()
