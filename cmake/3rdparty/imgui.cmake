add_library(imgui STATIC
    ${project_root}/3rdparty/imgui/imgui.cpp
    ${project_root}/3rdparty/imgui/imgui_draw.cpp
    ${project_root}/3rdparty/imgui/imgui_widgets.cpp
)

target_include_directories(imgui
    PUBLIC
        ${project_root}/3rdparty/imgui
)

set_target_properties(imgui PROPERTIES FOLDER "3rdparty/imgui")
