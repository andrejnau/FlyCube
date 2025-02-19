set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

if (MSVC)
    add_compile_definitions(
        UNICODE
        _UNICODE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
    )
    add_compile_options(
        /MP
        /wd4005
        /wd4838
        /wd5051
    )
else()
    add_compile_options(
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-deprecated-declarations>
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-shorten-64-to-32>
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-unguarded-availability-new>
    )
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    add_compile_definitions(TARGET_IOS=1)
elseif (CMAKE_SYSTEM_NAME STREQUAL "tvOS")
    add_compile_definitions(TARGET_TVOS=1)
else()
    add_compile_definitions(TARGET_MACOS=1)
endif()

if (CMAKE_SIZEOF_VOID_P STREQUAL "4")
    message(FATAL_ERROR "x86 build is not supported")
endif()
