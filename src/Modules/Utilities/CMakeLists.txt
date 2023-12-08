list(APPEND headers
    Common.h
    DXUtility.h
    SystemUtils.h
    ScopeGuard.h
    VKUtility.h
    DXGIFormatHelper.h
    FormatHelper.h
)
list(APPEND sources
    SystemUtils.cpp
    DXGIFormatHelper.cpp
    FormatHelper.cpp
)

if (APPLE)
    list(APPEND headers
        AutoreleasePool.h
    )
    list(APPEND sources
        AutoreleasePool.mm
    )
    set_property(SOURCE AutoreleasePool.mm PROPERTY COMPILE_FLAGS "-fno-objc-arc")
endif()

add_library(Utilities ${headers} ${sources})

target_link_libraries(Utilities
    gli
    glm
    DirectX-Headers
    nowide
)

if (APPLE)
    target_link_libraries(Utilities
        "-framework Foundation"
    )
endif()

target_include_directories(Utilities
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

set_target_properties(Utilities PROPERTIES FOLDER "Modules")
