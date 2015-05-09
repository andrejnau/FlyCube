set (PROJECT CGPhysical)
project (${PROJECT})

set (HEADERS
    scene.h
    geometry.h
    shaders.h)

set (SOURCES
    main.cpp)

if (MSVC)
    add_executable(${PROJECT} WIN32 ${SOURCES} ${HEADERS})
    set_target_properties(${PROJECT} PROPERTIES
                        LINK_FLAGS "/ENTRY:mainCRTStartup")
else()
    add_executable(${PROJECT} ${SOURCES} ${HEADERS})
endif()

target_link_libraries(${PROJECT} glfw ${GLFW_LIBRARIES} gl_core mlogger SOIL modern_gl_camera)