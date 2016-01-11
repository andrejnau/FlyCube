option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
option(GLFW_INSTALL "Generate installation target" OFF)

add_subdirectory(${PROJECT_ROOT}/3rdparty/glfw-3.1.2)
set_target_properties(glfw PROPERTIES FOLDER "3rdparty/glfw")
include_directories(${PROJECT_ROOT}/3rdparty/glfw-3.1.2/include)
add_definitions("-DGLFW_INCLUDE_NONE")