set(target VulkanExtLoader)

set(headers
    VulkanExtLoader.h
)

set(sources
    VulkanExtLoader.cpp
)

add_library(${target} ${headers} ${sources})

target_link_libraries(${target}
    ${Vulkan_LIBRARIES}
)

target_include_directories(${target}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
        ${Vulkan_INCLUDE_DIR}
)

set_target_properties(${target} PROPERTIES FOLDER "library")
