if (NOT METAL_SUPPORT)
    return()
endif()

add_subdirectory("${project_root}/3rdparty/MVKPixelFormats" EXCLUDE_FROM_ALL)
set(appleclang_compile_options
    -Wno-deprecated-declarations
)
target_compile_options(MVKPixelFormats
    PUBLIC
        "$<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:${appleclang_compile_options}>"
)

set_target_properties(MVKPixelFormats PROPERTIES FOLDER "3rdparty/MVKPixelFormats")
