add_executable(ShaderReflectionTest main.cpp)
target_link_libraries(ShaderReflectionTest PRIVATE ShaderReflection Catch2::Catch2)
set_target_properties(ShaderReflectionTest PROPERTIES FOLDER "Tests")
