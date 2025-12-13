list(APPEND sources
    main.cpp
)

set(shaders_path "${assets_path}/shaders/BufferViewTest")

set(vertex_shaders ${shaders_path}/VertexShader.hlsl)
set_property(SOURCE ${vertex_shaders} PROPERTY SHADER_TYPE Vertex)
set(pixel_shaders ${shaders_path}/PixelShader.hlsl)
set_property(SOURCE ${pixel_shaders} PROPERTY SHADER_TYPE Pixel)

set(shaders_files ${vertex_shaders} ${pixel_shaders})
set_property(SOURCE ${shaders_files} PROPERTY SHADER_ENTRYPOINT main)
set_property(SOURCE ${shaders_files} PROPERTY SHADER_MODEL 6_0)

include(compile_shaders)
compile_shaders("${shaders_path}" "assets/BufferViewTest" "${shaders_files}" resources)

if (ANDROID)
    add_library(BufferViewTest SHARED
        ${resources}
        ${shaders_files}
        ${sources}
    )
else()
    add_executable(BufferViewTest WIN32 MACOSX_BUNDLE
        ${resources}
        ${shaders_files}
        ${sources}
    )
endif()

target_link_options(BufferViewTest
    PRIVATE
        $<$<BOOL:${WIN32}>:/ENTRY:mainCRTStartup>
)

target_link_libraries(BufferViewTest
    AppLoop
    FlyCube
)

if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set_target_properties(BufferViewTest PROPERTIES
        XCODE_EXPLICIT_FILE_TYPE "wrapper.application"
    )
endif()

set_target_properties(BufferViewTest PROPERTIES
    FOLDER "Apps"
    MACOSX_BUNDLE_INFO_PLIST "${project_root}/cmake/apple/Info.plist"
)
