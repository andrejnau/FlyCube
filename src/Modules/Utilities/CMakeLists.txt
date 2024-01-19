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

add_library(Utilities ${headers} ${sources})

target_link_libraries(Utilities
    gli
    glm
    DirectX-Headers
    nowide
)

target_include_directories(Utilities
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

set_target_properties(Utilities PROPERTIES FOLDER "Modules")
