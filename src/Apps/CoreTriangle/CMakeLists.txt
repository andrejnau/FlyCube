add_executable(CoreTriangle WIN32 main.cpp)

if (WIN32)
    set_target_properties(CoreTriangle PROPERTIES
        LINK_FLAGS "/ENTRY:mainCRTStartup"
    )
endif()

target_link_libraries(CoreTriangle
    AppBox
    Instance
)

set_target_properties(CoreTriangle PROPERTIES FOLDER "Apps")

install(TARGETS CoreTriangle)
