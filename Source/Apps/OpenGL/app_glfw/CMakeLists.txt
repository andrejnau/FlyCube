set (target OpenGLApp)

set (SOURCES
    main.cpp)

if (MSVC)
    add_executable(${target} WIN32 ${SOURCES})
    set_target_properties(${target} PROPERTIES
                        LINK_FLAGS "/ENTRY:mainCRTStartup")
else()
    add_executable(${target} ${SOURCES})
endif()

target_link_libraries(${target} AppBox demo_scene)
