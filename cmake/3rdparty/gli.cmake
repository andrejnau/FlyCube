add_library(gli INTERFACE)
target_include_directories(gli INTERFACE "${project_root}/3rdparty/gli")

set(appleclang_clang_compile_options
    -Wno-deprecated-anon-enum-enum-conversion
)
set(appleclang_compile_options
    -Wno-missing-template-arg-list-after-template-kw
)
set(gnu_compile_options
    -Wno-deprecated-enum-enum-conversion
)
target_compile_options(gli
    INTERFACE
        "$<$<COMPILE_LANG_AND_ID:CXX,AppleClang,Clang>:${appleclang_clang_compile_options}>"
        "$<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:${appleclang_compile_options}>"
        "$<$<COMPILE_LANG_AND_ID:CXX,GNU>:${gnu_compile_options}>"
)
