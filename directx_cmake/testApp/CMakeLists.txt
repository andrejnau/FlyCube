set (PROJECT testApp)
project (${PROJECT})

add_executable(${PROJECT} main.cpp)

include_directories (
    ${CMAKE_CURRENT_SOURCE_DIR}/../d3dcommon
    ${DirectX_D3D11_1_INCLUDE_DIR}
)

link_libraries (
    ${DirectX_D3D11_LIBRARY}
)