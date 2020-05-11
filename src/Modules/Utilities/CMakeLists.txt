add_library(Utilities
    DXUtility.h
    FileUtility.h
    Logger.h
    ScopeGuard.h
    Singleton.h
    State.cpp
    State.h
    VKUtility.h
    Vulkan.h
)

target_include_directories(Utilities
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

set_target_properties(Utilities PROPERTIES FOLDER "library")
