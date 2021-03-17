set(target DispatchIndirect)

set(include_path "${CMAKE_CURRENT_SOURCE_DIR}")
set(source_path "${CMAKE_CURRENT_SOURCE_DIR}")
set(shaders_path "${assets_path}/shaders/DispatchIndirect")

set(sources
    ${source_path}/main.cpp
)

set(compute_shaders
    ${shaders_path}/Noise.hlsl
    ${shaders_path}/Task.hlsl
)

set_property(SOURCE ${compute_shaders} PROPERTY VS_SHADER_ENTRYPOINT MainCS)
set_property(SOURCE ${compute_shaders} PROPERTY VS_SHADER_TYPE Compute)
set_property(SOURCE ${compute_shaders} PROPERTY VS_SHADER_MODEL 6.5)
set_property(SOURCE ${compute_shaders} PROPERTY VS_SHADER_FLAGS "/Zi /auto-binding-space 0")

set(shaders_files ${compute_shaders})

include(${project_root}/3rdparty/FlyCube/cmake/shader_parser.cmake)

source_group("Shader Files" FILES ${shaders_files})
source_group("Shader Ref Files" FILES ${shaders_ref})

add_executable(${target} WIN32 ${headers} ${sources} ${shaders_files} ${shaders_ref})
gen_shaders_ref(${target} "${shaders_files}" shaders_ref)

if (WIN32)
    set_target_properties(${target} PROPERTIES
                            LINK_FLAGS "/ENTRY:mainCRTStartup")
endif()

target_include_directories(${target}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
        ${CMAKE_BINARY_DIR}/gen/${target}
)

target_link_libraries(${target}
    AppBox
    RenderDevice
    Texture
)

set_target_properties(${target} PROPERTIES FOLDER "Apps")

install(TARGETS ${target})
