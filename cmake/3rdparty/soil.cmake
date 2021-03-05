add_subdirectory(${project_root}/3rdparty/soil soil EXCLUDE_FROM_ALL)
target_include_directories(SOIL INTERFACE "${project_root}/3rdparty/soil/src")
set_target_properties(SOIL PROPERTIES FOLDER "3rdparty/soil")
