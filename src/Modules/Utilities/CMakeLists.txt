add_library(Utilities
    DXUtility.h
    FileUtility.h
    Logger.h
    ScopeGuard.h
    Singleton.h
    VKUtility.h
    Vulkan.h
    DXGIFormatHelper.cpp
    DXGIFormatHelper.h
    FormatHelper.cpp
    FormatHelper.h
)

target_include_directories(Utilities
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

set_target_properties(Utilities PROPERTIES FOLDER "Modules")
