set (PROJECT gl_camera)
project (${PROJECT})

set (HEADERS include/camera.h)
set (SOURCES src/camera.cpp)

add_library(${PROJECT} ${HEADERS} ${SOURCES})
target_link_libraries(${PROJECT} ${OPENGL_LIBRARIES})

set_target_properties(${PROJECT} PROPERTIES FOLDER "common")