add_library(pix3 SHARED IMPORTED)
set_target_properties(pix3 PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${project_root}/3rdparty/WinPixEventRuntime/include"
    IMPORTED_IMPLIB "${project_root}/3rdparty/WinPixEventRuntime/bin/WinPixEventRuntime.lib"
    IMPORTED_LOCATION "${project_root}/3rdparty/WinPixEventRuntime/bin/WinPixEventRuntime.dll"
    INTERFACE_COMPILE_DEFINITIONS "USE_PIX"
)

set(trigger_dir "${CMAKE_BINARY_DIR}/copy")
set(trigger_file "${trigger_dir}/pix3")

add_custom_command(OUTPUT ${trigger_file}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${trigger_dir} 
    COMMAND ${CMAKE_COMMAND} -E touch ${trigger_file} 
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:pix3>
        $<TARGET_FILE_DIR:SponzaPbr>/$<TARGET_FILE_NAME:pix3>
)

add_custom_target(pix3_copy ALL DEPENDS ${trigger_file})
set_target_properties(pix3_copy PROPERTIES FOLDER "3rdparty/copy")
