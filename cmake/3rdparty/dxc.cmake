add_library(dxc INTERFACE)
target_include_directories(dxc INTERFACE "${project_root}/3rdparty/dxc/include")
target_compile_definitions(dxc INTERFACE DXC_CUSTOM_LOCATION="${project_root}/3rdparty/dxc/bin")
target_compile_definitions(dxc INTERFACE DXC_DEFAULT_LOCATION="C:/Program Files (x86)/Windows Kits/10/Bin/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}")

list(APPEND CMAKE_MODULE_PATH "${project_root}/3rdparty/dxc/cmake/modules")
find_package(DiaSDK REQUIRED)
add_library(dia INTERFACE)
target_include_directories(dia INTERFACE "${DIASDK_INCLUDE_DIRS}")
