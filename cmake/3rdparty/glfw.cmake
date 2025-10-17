if (IOS_OR_TVOS OR ANDROID)
    return()
endif()

option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
option(GLFW_INSTALL "Generate installation target" OFF)

add_subdirectory(${project_root}/3rdparty/glfw glfw EXCLUDE_FROM_ALL)
set_target_properties(glfw PROPERTIES FOLDER "3rdparty/glfw")
set_target_properties(update_mappings PROPERTIES FOLDER "3rdparty/glfw")
target_include_directories(glfw INTERFACE "${project_root}/3rdparty/glfw/include")
target_compile_definitions(glfw INTERFACE GLFW_INCLUDE_NONE)
