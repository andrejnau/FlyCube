add_subdirectory(${project_root}/3rdparty/Vulkan-Headers Vulkan-Headers EXCLUDE_FROM_ALL)

add_library(vulkan INTERFACE)
target_link_libraries(vulkan INTERFACE Vulkan-Headers)

set_property(TARGET vulkan APPEND PROPERTY
    INTERFACE_COMPILE_DEFINITIONS
        DVULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
        VK_NO_PROTOTYPES
)

if (WIN32)
    set_property(TARGET vulkan APPEND PROPERTY
        INTERFACE_COMPILE_DEFINITIONS
            VK_USE_PLATFORM_WIN32_KHR
    )
elseif(APPLE)
    set_property(TARGET vulkan APPEND PROPERTY
        INTERFACE_COMPILE_DEFINITIONS
            VK_USE_PLATFORM_MACOS_MVK
            VK_USE_PLATFORM_METAL_EXT
    )
    set(CMAKE_XCODE_GENERATE_SCHEME ON)
    set(CMAKE_XCODE_SCHEME_ENVIRONMENT
        "VULKAN_SDK=$ENV{VULKAN_SDK}"
    )
else()
    set_property(TARGET vulkan APPEND PROPERTY
        INTERFACE_COMPILE_DEFINITIONS
            VK_USE_PLATFORM_XCB_KHR
    )
endif()
