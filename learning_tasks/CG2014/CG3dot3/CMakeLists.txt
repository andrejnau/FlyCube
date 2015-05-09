set (PROJECT CG3dot3)
project (${PROJECT})

set (HEADERS
    scene.h
    geometry.h
    shaders.h)

set (SOURCES
    main.cpp)

include_directories(../../../core_source/include)

if (MSVC)
    add_executable(${PROJECT} ${SOURCES} ${HEADERS})
    set_target_properties(${PROJECT} PROPERTIES
                        LINK_FLAGS "/ENTRY:mainCRTStartup")
else()
    add_executable(${PROJECT} ${SOURCES} ${HEADERS})
endif()

target_link_libraries(${PROJECT} glfw ${GLFW_LIBRARIES} gl_core mlogger SOIL modern_gl_camera)