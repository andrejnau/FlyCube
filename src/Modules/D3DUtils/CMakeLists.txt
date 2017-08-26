set(target D3DUtils)

set(include_path "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(source_path "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(shaders_path "${assets_path}/shaders/DX12")

set(headers
    ${include_path}/FileUtility.h
    ${include_path}/Util.h
    ${include_path}/Utility.h
    ${include_path}/Geometry.h
)

set(sources
    ${source_path}/Util.cpp
    ${source_path}/Geometry.cpp
)

add_library(${target} ${headers} ${sources})

target_compile_definitions(${target} PRIVATE UNICODE _UNICODE NOMINMAX)

target_include_directories(${target}
    PUBLIC
        ${include_path}
)

set_target_properties(${target} PROPERTIES FOLDER "library")
