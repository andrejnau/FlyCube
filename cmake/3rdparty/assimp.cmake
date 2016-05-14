option (ASSIMP_BUILD_ASSIMP_TOOLS
  "If the supplementary tools for Assimp are built in addition to the library."
  OFF
)

option (ASSIMP_BUILD_TESTS
  "If the test suite for Assimp is built in addition to the library."
  OFF
)

option (ASSIMP_NO_EXPORT
  "Disable Assimp's export functionality."
  ON
)

add_subdirectory(${project_root}/3rdparty/assimp-3.2 assimp)
include_directories(${project_root}/3rdparty/assimp-3.2/code/BoostWorkaround)
include_directories(${project_root}/3rdparty/assimp-3.2/include)
set_target_properties(assimp PROPERTIES FOLDER "3rdparty/assimp")
set_target_properties(zlibstatic PROPERTIES FOLDER "3rdparty/assimp")
set_target_properties(uninstall PROPERTIES FOLDER "3rdparty/assimp")