add_subdirectory(AppBox)

if(BUILD_DX11)
    add_subdirectory(D3D11)
endif()

if(BUILD_DX12)
    add_subdirectory(D3D12)
endif()

if (BUILD_OPENGL)
    add_subdirectory(OpenGL)
endif()
