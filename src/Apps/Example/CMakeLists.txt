add_executable(Example WIN32 main.cpp)

if (WIN32)
    set_target_properties(Example PROPERTIES
        LINK_FLAGS "/ENTRY:mainCRTStartup"
    )
endif()

target_link_libraries(Example
    AppBox
    Instance
)
