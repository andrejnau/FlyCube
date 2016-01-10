add_subdirectory(${PROJECT_ROOT}/3rdparty/Simple-OpenGL-Image-Library-master)
include_directories(${PROJECT_ROOT}/3rdparty/Simple-OpenGL-Image-Library-master/src)
set_target_properties(SOIL PROPERTIES FOLDER "3rdparty/soil")