add_library(Utilities
    Common.h
    DXUtility.h
    FileUtility.h
    FileUtility.cpp
    Logger.h
    ScopeGuard.h
    Singleton.h
    VKUtility.h
    DXGIFormatHelper.cpp
    DXGIFormatHelper.h
    FormatHelper.cpp
    FormatHelper.h
)

target_link_libraries(Utilities
    gli
    glm
    DirectX-Headers
)

target_include_directories(Utilities
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

set_target_properties(Utilities PROPERTIES FOLDER "Modules")
