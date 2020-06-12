add_library(imgui STATIC
    ${project_root}/3rdparty/unpacked/imgui-1.52/imgui.cpp
    ${project_root}/3rdparty/unpacked/imgui-1.52/imgui_draw.cpp
    ${project_root}/3rdparty/unpacked/imgui-1.52/imgui_demo.cpp
)

target_include_directories(imgui
    PUBLIC
        ${project_root}/3rdparty/unpacked/imgui-1.52
)

set_target_properties(imgui PROPERTIES FOLDER "3rdparty/imgui")
