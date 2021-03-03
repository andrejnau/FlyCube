find_package(Vulkan REQUIRED)

add_library(vulkan STATIC IMPORTED)
set_target_properties(vulkan PROPERTIES
    IMPORTED_LOCATION "${Vulkan_LIBRARIES}"
)
set_target_properties(vulkan PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${Vulkan_INCLUDE_DIR}"
)

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
else()
    set_property(TARGET vulkan APPEND PROPERTY
        INTERFACE_COMPILE_DEFINITIONS
            VK_USE_PLATFORM_XCB_KHR
    )
endif()
