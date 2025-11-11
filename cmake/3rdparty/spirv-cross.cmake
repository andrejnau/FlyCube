option(SPIRV_CROSS_CLI "Build the CLI binary. Requires SPIRV_CROSS_STATIC." OFF)
option(SPIRV_CROSS_ENABLE_C_API "Enable C API wrapper support in static library." OFF)
option(SPIRV_CROSS_ENABLE_CPP "Enable C++ target support." OFF)
option(SPIRV_CROSS_ENABLE_REFLECT "Enable JSON reflection target support." OFF)
option(SPIRV_CROSS_ENABLE_TESTS "Enable SPIRV-Cross tests." OFF)
option(SPIRV_CROSS_ENABLE_UTIL "Enable util module support." OFF)
option(SPIRV_CROSS_SKIP_INSTALL "Skips installation targets." ON)

add_subdirectory(${project_root}/3rdparty/SPIRV-Cross EXCLUDE_FROM_ALL)
target_compile_options(spirv-cross-msl
    PRIVATE
        $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/wd5051>
)

set_target_properties(spirv-cross-core PROPERTIES FOLDER "3rdparty/spirv-cross")
set_target_properties(spirv-cross-glsl PROPERTIES FOLDER "3rdparty/spirv-cross")
set_target_properties(spirv-cross-hlsl PROPERTIES FOLDER "3rdparty/spirv-cross")
set_target_properties(spirv-cross-msl PROPERTIES FOLDER "3rdparty/spirv-cross")
