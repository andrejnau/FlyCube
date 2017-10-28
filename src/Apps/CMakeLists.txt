if(BUILD_DX11)
    add_subdirectory(D3D11)
    add_subdirectory(D3D12)
endif()

if (BUILD_OPENGL)
    add_subdirectory(OpenGL)
    add_subdirectory(GLDemo)
endif()
