add_executable(MetalTestApp WIN32 main.cpp)

if (WIN32)
    set_target_properties(MetalTestApp PROPERTIES
        LINK_FLAGS "/ENTRY:mainCRTStartup"
    )
endif()

target_link_libraries(MetalTestApp
    AppBox
    Instance
)

set_target_properties(MetalTestApp PROPERTIES FOLDER "Apps")

install(TARGETS MetalTestApp)
