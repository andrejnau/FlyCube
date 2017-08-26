add_subdirectory(demo_scene)
add_subdirectory(utils)
add_subdirectory(simple_camera)

if (NOT ANDROID)
    add_subdirectory(gl_load)
endif()