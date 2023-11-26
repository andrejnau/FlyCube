if (NOT BUILD_TESTING)
    return()
endif()

add_subdirectory(${project_root}/3rdparty/Catch2)
set_target_properties(Catch2 PROPERTIES FOLDER "3rdparty/catch2")
set_target_properties(Catch2WithMain PROPERTIES FOLDER "3rdparty/catch2")
