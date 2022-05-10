add_subdirectory(${project_root}/3rdparty/Vulkan-Headers Vulkan-Headers EXCLUDE_FROM_ALL)

add_library(vulkan INTERFACE)
target_link_libraries(vulkan INTERFACE Vulkan-Headers)

if (APPLE)
    set_property(TARGET vulkan APPEND PROPERTY
        INTERFACE_COMPILE_DEFINITIONS
            USE_STATIC_MOLTENVK
    )
    set(moltenvk_xcframework "$ENV{VULKAN_SDK}/MoltenVK/MoltenVK.xcframework")
    set(moltenvk_copy_dir "${CMAKE_CURRENT_BINARY_DIR}/moltenvk")
    set(moltenvk_copy_status "${moltenvk_copy_dir}/status")
    add_custom_command(OUTPUT "${moltenvk_copy_status}"
        COMMAND cmake -E make_directory "${moltenvk_copy_dir}"
        COMMAND cmake -E create_symlink "${moltenvk_xcframework}/ios-arm64/libMoltenVK.a" "${moltenvk_copy_dir}/iOS-iphoneos-libMoltenVK.a"
        COMMAND cmake -E create_symlink "${moltenvk_xcframework}/ios-arm64_x86_64-simulator/libMoltenVK.a" "${moltenvk_copy_dir}/iOS-iphonesimulator-libMoltenVK.a"
        COMMAND cmake -E create_symlink "${moltenvk_xcframework}/macos-arm64_x86_64/libMoltenVK.a" "${moltenvk_copy_dir}/Darwin-libMoltenVK.a"
        COMMAND cmake -E create_symlink "${moltenvk_xcframework}/tvos-arm64_arm64e/libMoltenVK.a" "${moltenvk_copy_dir}/tvOS-appletvos-libMoltenVK.a"
        COMMAND cmake -E create_symlink "${moltenvk_xcframework}/tvos-arm64_x86_64-simulator/libMoltenVK.a" "${moltenvk_copy_dir}/tvOS-appletvsimulator-libMoltenVK.a"
        COMMAND cmake -E touch "${moltenvk_copy_status}"
    )
    add_custom_target(moltenvk_copy_ckeck DEPENDS ${moltenvk_copy_status})
    add_dependencies(vulkan moltenvk_copy_ckeck)
    if (CMAKE_GENERATOR STREQUAL "Xcode")
        set_property(TARGET vulkan APPEND PROPERTY
            INTERFACE_LINK_LIBRARIES
                "${moltenvk_copy_dir}/$<PLATFORM_ID>$(EFFECTIVE_PLATFORM_NAME)-libMoltenVK.a"
        )
    else()
        set_property(TARGET vulkan APPEND PROPERTY
            INTERFACE_LINK_LIBRARIES
                "${moltenvk_copy_dir}/$<PLATFORM_ID>-libMoltenVK.a"
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
            VK_USE_PLATFORM_IOS_MVK
            VK_USE_PLATFORM_MACOS_MVK
    )
else()
    set_property(TARGET vulkan APPEND PROPERTY
        INTERFACE_COMPILE_DEFINITIONS
            VK_USE_PLATFORM_XCB_KHR
    )
endif()
