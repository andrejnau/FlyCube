set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF)
set(ASSIMP_BUILD_GLTF_IMPORTER ON)
set(ASSIMP_BUILD_OBJ_IMPORTER ON)
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_INSTALL OFF)
set(ASSIMP_INSTALL_PDB OFF)
set(ASSIMP_NO_EXPORT ON)

add_subdirectory(${project_root}/3rdparty/assimp assimp EXCLUDE_FROM_ALL)

set_target_properties(assimp PROPERTIES FOLDER "3rdparty/assimp")

if (TARGET zlibstatic)
    set_target_properties(zlibstatic PROPERTIES FOLDER "3rdparty/assimp")
endif()
