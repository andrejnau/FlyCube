add_subdirectory(${project_root}/3rdparty/Simple-OpenGL-Image-Library-master Simple-OpenGL-Image-Library)
include_directories(${project_root}/3rdparty/Simple-OpenGL-Image-Library-master/src)
set_target_properties(SOIL PROPERTIES FOLDER "3rdparty/soil")