set (PROJECT LoadTexture)
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

target_link_libraries(${PROJECT} glfw glbinding mlogger SOIL utils)

set_target_properties(${PROJECT} PROPERTIES FOLDER "samples")