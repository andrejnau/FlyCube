set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

set(appleclang_clang_compile_options
    -Wno-deprecated-anon-enum-enum-conversion
)
set(appleclang_compile_options
    -Wno-deprecated-declarations
    -Wno-deprecated-volatile
    -Wno-shorten-64-to-32
    -Wno-unguarded-availability-new
)
set(gpu_compile_options
    -Wno-deprecated-enum-enum-conversion
)
set(msvc_compile_options
    /MP
    /wd4005
    /wd4838
    /wd5051
)
add_compile_options(
    "$<$<COMPILE_LANG_AND_ID:CXX,AppleClang,Clang>:${appleclang_clang_compile_options}>"
    "$<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:${appleclang_compile_options}>"
    "$<$<COMPILE_LANG_AND_ID:CXX,GNU>:${gpu_compile_options}>"
    "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:${msvc_compile_options}>"
)

set(msvc_compile_definitions
    _CRT_SECURE_NO_WARNINGS
    _UNICODE
    NOMINMAX
    UNICODE
)
set(darwin_compile_definitions
    TARGET_MACOS=1
)
set(ios_compile_definitions
    TARGET_IOS=1
)
set(tvos_compile_definitions
    TARGET_TVOS=1
)
add_compile_definitions(
    "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:${msvc_compile_definitions}>"
    "$<$<PLATFORM_ID:Darwin>:${darwin_compile_definitions}>"
    "$<$<PLATFORM_ID:iOS>:${ios_compile_definitions}>"
    "$<$<PLATFORM_ID:tvOS>:${tvos_compile_definitions}>"
)
