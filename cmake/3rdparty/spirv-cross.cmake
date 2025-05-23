option(SPIRV_CROSS_CLI "Build the CLI binary. Requires SPIRV_CROSS_STATIC." OFF)
option(SPIRV_CROSS_ENABLE_TESTS "Enable SPIRV-Cross tests." OFF)

add_subdirectory(${project_root}/3rdparty/SPIRV-Cross EXCLUDE_FROM_ALL)
target_compile_options(spirv-cross-msl
    PRIVATE
        $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/wd5051>
)

set_target_properties(spirv-cross-core PROPERTIES FOLDER "3rdparty/spirv-cross")
set_target_properties(spirv-cross-cpp PROPERTIES FOLDER "3rdparty/spirv-cross")
set_target_properties(spirv-cross-glsl PROPERTIES FOLDER "3rdparty/spirv-cross")
set_target_properties(spirv-cross-hlsl PROPERTIES FOLDER "3rdparty/spirv-cross")
set_target_properties(spirv-cross-msl PROPERTIES FOLDER "3rdparty/spirv-cross")
set_target_properties(spirv-cross-reflect PROPERTIES FOLDER "3rdparty/spirv-cross")
set_target_properties(spirv-cross-util PROPERTIES FOLDER "3rdparty/spirv-cross")
set_target_properties(spirv-cross-c PROPERTIES FOLDER "3rdparty/spirv-cross")
