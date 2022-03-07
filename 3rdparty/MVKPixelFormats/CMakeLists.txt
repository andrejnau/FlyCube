add_library(MVKPixelFormats STATIC
    MVKPixelFormats.h
    MVKPixelFormats.mm
)

target_include_directories(MVKPixelFormats
    PUBLIC
        .
)

target_link_libraries(MVKPixelFormats
    spirv-cross-msl
    vulkan
    "-framework Metal"
)

target_compile_definitions(MVKPixelFormats PUBLIC MVK_MACOS)
