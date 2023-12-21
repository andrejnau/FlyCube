add_subdirectory(${project_root}/3rdparty/Vulkan-Headers Vulkan-Headers EXCLUDE_FROM_ALL)

add_library(vulkan INTERFACE)
target_link_libraries(vulkan INTERFACE Vulkan-Headers)

if (NOT VULKAN_SUPPORT)
    return()
endif()

if (STATIC_MOLTEN_VK)
    if (CMAKE_VERSION VERSION_LESS "3.28")
        message(FATAL_ERROR "CMake 3.28 is required for linking with MoltenVK.xcframework")
    endif()

    if (CUSTOM_MOLTENVK)
        set(moltenvk_xcframework "${CUSTOM_MOLTENVK}/MoltenVK.xcframework")
    elseif(DEFINED ENV{VULKAN_SDK})
        set(moltenvk_xcframework "$ENV{VULKAN_SDK}/MoltenVK/MoltenVK.xcframework")
    else()
        message(FATAL_ERROR "MoltenVK.xcframework is missing")
    endif()
    message("MoltenVK.xcframework location is ${moltenvk_xcframework}")

    set_property(TARGET vulkan APPEND PROPERTY
        INTERFACE_LINK_LIBRARIES
            "${moltenvk_xcframework}"
    )
    set_property(TARGET vulkan APPEND PROPERTY
        INTERFACE_LINK_LIBRARIES
            "-framework IOSurface"
    )
    set_property(TARGET vulkan APPEND PROPERTY
        INTERFACE_COMPILE_DEFINITIONS
            USE_STATIC_MOLTENVK
    )
else()
    set_property(TARGET vulkan APPEND PROPERTY
        INTERFACE_COMPILE_DEFINITIONS
            DVULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
            VK_NO_PROTOTYPES
    )
endif()

if (WIN32)
    set_property(TARGET vulkan APPEND PROPERTY
        INTERFACE_COMPILE_DEFINITIONS
            VK_USE_PLATFORM_WIN32_KHR
    )
elseif(APPLE)
    set_property(TARGET vulkan APPEND PROPERTY
        INTERFACE_COMPILE_DEFINITIONS
            VK_USE_PLATFORM_METAL_EXT
    )
    set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
    set(CMAKE_INSTALL_RPATH "/usr/local/lib")
elseif(NOT ANDROID)
    set_property(TARGET vulkan APPEND PROPERTY
        INTERFACE_COMPILE_DEFINITIONS
            VK_USE_PLATFORM_XCB_KHR
    )
endif()
