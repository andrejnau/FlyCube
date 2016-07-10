set (PROJECT Demo)
project (${PROJECT})

add_subdirectory(scene)

set (SOURCES
    main.cpp)

if (MSVC)
    add_executable(${PROJECT} WIN32 ${SOURCES})
    set_target_properties(${PROJECT} PROPERTIES
                        LINK_FLAGS "/ENTRY:mainCRTStartup")
else()
    add_executable(${PROJECT} ${SOURCES})
endif()

target_link_libraries(${PROJECT} glfw ${GLFW_LIBRARIES} gl_load SOIL assimp simple_camera utils DemoScene)

set_target_properties(${PROJECT} PROPERTIES FOLDER "samples")