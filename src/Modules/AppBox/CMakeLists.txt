set(target AppBox)

set(headers
    AppBox.h
)

set(sources
    AppBox.cpp
)

add_library(${target} ${headers} ${sources})

target_link_libraries(${target}
    glfw
	Context
)

if (VULKAN_SUPPORT)
    target_link_libraries(${target}
        ${Vulkan_LIBRARIES}
    )
endif()

target_include_directories(${target}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
        ${Vulkan_INCLUDE_DIR}
)

set_target_properties(${target} PROPERTIES FOLDER "library")
