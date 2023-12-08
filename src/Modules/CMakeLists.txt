if (NOT IOS_OR_TVOS)
    add_subdirectory(AppBox)
endif()

if (CMAKE_GENERATOR MATCHES "Xcode")
    add_subdirectory(AppBoxUIKit)
endif()

add_subdirectory(Utilities)
