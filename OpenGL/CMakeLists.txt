set (PROJECT FlyCube)
project (${PROJECT})

#init 3rdparty library
include(${project_root}/cmake/3rdparty/glfw.cmake)
include(${project_root}/cmake/3rdparty/soil.cmake)
include(${project_root}/cmake/3rdparty/assimp.cmake)
include(${project_root}/cmake/3rdparty/glm.cmake)
include(${project_root}/cmake/3rdparty/gli.cmake)

add_subdirectory(src)
add_subdirectory(app_glfw)