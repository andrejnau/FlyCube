option(BUILD_SHARED_LIBS     "Build shared instead of static libraries."              ON)
option(OPTION_BUILD_TESTS    "Build tests."                                           OFF)

add_subdirectory(${project_root}/3rdparty/glbinding-2.1.0 glbinding)

set_target_properties(generate PROPERTIES FOLDER "3rdparty/glbinding")
set_target_properties(glbinding PROPERTIES FOLDER "3rdparty/glbinding")
set_target_properties(pack PROPERTIES FOLDER "3rdparty/glbinding")
set_target_properties(pack-glbinding PROPERTIES FOLDER "3rdparty/glbinding")
set_target_properties(update PROPERTIES FOLDER "3rdparty/glbinding")