add_subdirectory(${project_root}/3rdparty/Vulkan-Headers Vulkan-Headers EXCLUDE_FROM_ALL)

add_library(vulkan INTERFACE)
target_link_libraries(vulkan INTERFACE Vulkan-Headers)

if (NOT VULKAN_SUPPORT)
    return()
endif()

if (STATIC_MOLTEN_VK AND APPLE)
    if (CMAKE_VERSION VERSION_LESS "3.28")
        message(FATAL_ERROR "CMake 3.28 is required for linking with MoltenVK.xcframework")
    endif()

    if (CUSTOM_MOLTENVK)
        list(APPEND moltenvk_xcframeworks
            "${CUSTOM_MOLTENVK}/MoltenVK.xcframework"
        )
    endif()
    if(DEFINED ENV{VULKAN_SDK})
        list(APPEND moltenvk_xcframeworks
            "$ENV{VULKAN_SDK}/lib/MoltenVK.xcframework"
            "$ENV{VULKAN_SDK}/macOS/lib/MoltenVK.xcframework"
            "$ENV{VULKAN_SDK}/MoltenVK/MoltenVK.xcframework"
        )
    endif()

    foreach(path ${moltenvk_xcframeworks})
        if (EXISTS "${path}")
            set(moltenvk_xcframework "${path}")
            break()
        endif()
    endforeach()

    if (EXISTS ${moltenvk_xcframework})
        message("MoltenVK.xcframework location is ${moltenvk_xcframework}")
    else()
        message(FATAL_ERROR "MoltenVK.xcframework is missing")
    endif()

    target_link_libraries(vulkan
        INTERFACE
            "-framework IOSurface"
            "-framework Metal"
            "-framework QuartzCore"
            "${moltenvk_xcframework}"
    )
    if (NOT IOS_OR_TVOS)
        target_link_libraries(vulkan
            INTERFACE
                "-framework AppKit"
                "-framework IOKit"
        )
    endif()
else()
    target_compile_definitions(vulkan
        INTERFACE
            VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
            VK_NO_PROTOTYPES
    )
endif()

if (WIN32)
    target_compile_definitions(vulkan
        INTERFACE
            VK_USE_PLATFORM_WIN32_KHR
    )
elseif (APPLE)
    target_compile_definitions(vulkan
        INTERFACE
            VK_USE_PLATFORM_METAL_EXT
    )
elseif (ANDROID)
    target_compile_definitions(vulkan
        INTERFACE
            VK_USE_PLATFORM_ANDROID_KHR
    )
else()
    target_compile_definitions(vulkan
        INTERFACE
            VK_USE_PLATFORM_XCB_KHR
            VK_USE_PLATFORM_XLIB_KHR
    )
    target_link_libraries(vulkan
        INTERFACE
            X11-xcb
    )
endif()
