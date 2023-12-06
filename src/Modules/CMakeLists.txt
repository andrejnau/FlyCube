if (IOS_OR_TVOS)
    add_subdirectory(AppBoxUIKit)
else()
    add_subdirectory(AppBox)
endif()

add_subdirectory(Utilities)
