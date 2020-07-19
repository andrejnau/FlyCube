add_subdirectory(${project_root}/3rdparty/soil soil EXCLUDE_FROM_ALL)
include_directories(${project_root}/3rdparty/soil/src)
set_target_properties(SOIL PROPERTIES FOLDER "3rdparty/soil")
