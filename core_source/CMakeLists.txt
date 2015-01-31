cmake_minimum_required (VERSION 2.8.12)

set (PROJECT core_source)
project (${PROJECT})

file(GLOB HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)

add_custom_target(
    ${PROJECT}
    ALL
    DEPENDS ${HEADERS}
    SOURCES ${HEADERS}
)