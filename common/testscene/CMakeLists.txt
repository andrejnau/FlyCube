set (PROJECT testscene)
project (${PROJECT})

file(GLOB HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)

add_custom_target(
    ${PROJECT}
    ALL
    DEPENDS ${HEADERS}
    SOURCES ${HEADERS}
)