set(ASSIMP_ENABLE_BOOST_WORKAROUND ON CACHE BOOL "If a simple implementation of the used Boost functions is used. Slightly reduces functionality, but enables builds without Boost available.")
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "If the official samples are built as well (needs Glut).")
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL	"If the supplementary tools for Assimp are built in addition to the library.")
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "If the test suite for Assimp is built in addition to the library.")

add_subdirectory(${PROJECT_ROOT}/3rdparty/assimp-3.1.1/)
include_directories(${PROJECT_ROOT}/3rdparty/assimp-3.1.1/code/BoostWorkaround)
include_directories(${PROJECT_ROOT}/3rdparty/assimp-3.1.1/include)
set_target_properties(assimp PROPERTIES FOLDER "3rdparty/assimp")
set_target_properties(zlibstatic PROPERTIES FOLDER "3rdparty/assimp")
set_target_properties(uninstall PROPERTIES FOLDER "3rdparty/assimp")