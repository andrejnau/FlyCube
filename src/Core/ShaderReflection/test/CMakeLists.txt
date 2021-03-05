add_executable(ShaderReflectionTest main.cpp)
if (WIN32)
    set_target_properties(ShaderReflectionTest PROPERTIES
        LINK_FLAGS "/ENTRY:wmainCRTStartup"
    )
endif()
target_link_libraries(ShaderReflectionTest PRIVATE ShaderReflection Catch2WithMain)
set_target_properties(ShaderReflectionTest PROPERTIES FOLDER "Tests")
