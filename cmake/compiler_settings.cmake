set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

if (MSVC)
    add_compile_options("/MP")
    add_definitions(-DUNICODE -D_UNICODE)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DNOMINMAX)
    add_compile_options(
        /wd4005
        /wd4715
        /wd4834
        /wd4838
    )
else()
    add_compile_options(
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-deprecated-declarations>
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-shorten-64-to-32>
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-switch>
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-unguarded-availability-new>
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-unused-result>
    )
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    add_definitions(-DTARGET_IOS=1)
elseif (CMAKE_SYSTEM_NAME STREQUAL "tvOS")
    add_definitions(-DTARGET_TVOS=1)
else()
    add_definitions(-DTARGET_MACOS=1)
endif()

if (CMAKE_SIZEOF_VOID_P STREQUAL "4")
    message(FATAL_ERROR "x86 build is not supported")
endif()
