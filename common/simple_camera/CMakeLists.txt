set (PROJECT simple_camera)
project (${PROJECT})

set (HEADERS include/simple_camera/camera.h)
set (SOURCES src/camera.cpp)

add_library(${PROJECT} ${HEADERS} ${SOURCES})
target_link_libraries(${PROJECT} glbinding)

set_target_properties(${PROJECT} PROPERTIES FOLDER "common")