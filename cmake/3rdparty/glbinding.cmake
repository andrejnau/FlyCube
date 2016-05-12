option(OPTION_SELF_CONTAINED "Create a self-contained install with all dependencies." OFF)
option(OPTION_BUILD_TESTS    "Build tests."                                           OFF)
option(OPTION_BUILD_DOCS     "Build documentation."                                   OFF)
option(OPTION_BUILD_EXAMPLES "Build examples."                                        OFF)
option(OPTION_BUILD_TOOLS    "Build tools."                                           OFF)
option(OPTION_EXTENSIVE_META "Provide extensive meta information for bidirectional conversion between String and OpenGL types" OFF)

add_subdirectory(${PROJECT_ROOT}/3rdparty/glbinding-master)

set_target_properties(generate PROPERTIES FOLDER "3rdparty/glbinding")
set_target_properties(glbinding PROPERTIES FOLDER "3rdparty/glbinding")
set_target_properties(pack PROPERTIES FOLDER "3rdparty/glbinding")
set_target_properties(pack-glbinding PROPERTIES FOLDER "3rdparty/glbinding")
set_target_properties(update PROPERTIES FOLDER "3rdparty/glbinding")