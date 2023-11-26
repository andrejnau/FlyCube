add_subdirectory(${project_root}/3rdparty/Vulkan-Headers Vulkan-Headers EXCLUDE_FROM_ALL)

add_library(vulkan INTERFACE)
target_link_libraries(vulkan INTERFACE Vulkan-Headers)

if (NOT VULKAN_SUPPORT)
    return()
endif()

if (APPLE)
    set_property(TARGET vulkan APPEND PROPERTY
        INTERFACE_COMPILE_DEFINITIONS
            USE_STATIC_MOLTENVK
    )

    if (DEFINED ENV{VULKAN_SDK})
        message("VULKAN_SDK is $ENV{VULKAN_SDK}")
    else()
        message(FATAL_ERROR "VULKAN_SDK is not set")
    endif()
    
    set(moltenvk_xcframework "$ENV{VULKAN_SDK}/MoltenVK/MoltenVK.xcframework")

    add_library(vulkan_libs_iOS-iphoneos UNKNOWN IMPORTED)
    set_property(TARGET vulkan_libs_iOS-iphoneos APPEND PROPERTY
        IMPORTED_LOCATION
            "${moltenvk_xcframework}/ios-arm64/libMoltenVK.a"
    )

    add_library(vulkan_libs_iOS-iphonesimulator UNKNOWN IMPORTED)
    set_property(TARGET vulkan_libs_iOS-iphonesimulator APPEND PROPERTY
        IMPORTED_LOCATION
            "${moltenvk_xcframework}/ios-arm64_x86_64-simulator/libMoltenVK.a"
    )

    add_library(vulkan_libs_Darwin UNKNOWN IMPORTED)
    set_property(TARGET vulkan_libs_Darwin APPEND PROPERTY
        IMPORTED_LOCATION
            "${moltenvk_xcframework}/macos-arm64_x86_64/libMoltenVK.a"
    )

    add_library(vulkan_libs_tvOS-appletvos UNKNOWN IMPORTED)
    set_property(TARGET vulkan_libs_tvOS-appletvos APPEND PROPERTY
        IMPORTED_LOCATION
            "${moltenvk_xcframework}/tvos-arm64_arm64e/libMoltenVK.a"
    )

    add_library(vulkan_libs_tvOS-appletvsimulator UNKNOWN IMPORTED)
    set_property(TARGET vulkan_libs_tvOS-appletvsimulator APPEND PROPERTY
        IMPORTED_LOCATION
            "${moltenvk_xcframework}/tvos-arm64_x86_64-simulator/libMoltenVK.a"
    )

    if (CMAKE_GENERATOR STREQUAL "Xcode")
        set_property(TARGET vulkan APPEND PROPERTY
            INTERFACE_LINK_LIBRARIES
                "vulkan_libs_$<PLATFORM_ID>$(EFFECTIVE_PLATFORM_NAME)"
        )
    else()
        set_property(TARGET vulkan APPEND PROPERTY
            INTERFACE_LINK_LIBRARIES
                "vulkan_libs_$<PLATFORM_ID>"
        )
    endif()
    set_property(TARGET vulkan APPEND PROPERTY
        INTERFACE_LINK_LIBRARIES
            "-framework IOSurface"
            "-framework CoreGraphics"
    )
    if (NOT IOS_OR_TVOS)
        set_property(TARGET vulkan APPEND PROPERTY
            INTERFACE_LINK_LIBRARIES
                "-framework IOKit"
        )
    endif()
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
else()
    set_property(TARGET vulkan APPEND PROPERTY
        INTERFACE_COMPILE_DEFINITIONS
            VK_USE_PLATFORM_XCB_KHR
    )
endif()
