if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${project_root}/install" CACHE PATH "install path" FORCE)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMakeTargets")
set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)
option(BUILD_SHARED_LIBS "Build package with shared libraries." OFF)

if (IOS)
    message(FATAL_ERROR "iOS is not supported")
endif()
