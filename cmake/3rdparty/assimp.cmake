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

add_subdirectory(${project_root}/3rdparty/assimp assimp EXCLUDE_FROM_ALL)

set_target_properties(assimp PROPERTIES FOLDER "3rdparty/assimp")

if (TARGET zlibstatic)
    set_target_properties(zlibstatic PROPERTIES FOLDER "3rdparty/assimp")
endif()

if (TARGET uninstall)
set_target_properties(uninstall PROPERTIES FOLDER "3rdparty/assimp")
endif()

if (TARGET UpdateAssimpLibsDebugSymbolsAndDLLs)
    set_target_properties(UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTIES FOLDER "3rdparty/assimp")
endif()

if (TARGET IrrXML)
set_target_properties(IrrXML PROPERTIES FOLDER "3rdparty/assimp")
endif()

if (TARGET zlib)
    set_target_properties(zlib PROPERTIES FOLDER "3rdparty/assimp")
endif()
