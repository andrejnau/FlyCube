add_library(stb_image
    "${project_root}/3rdparty/stb_image/stb_image.c"
    "${project_root}/3rdparty/stb_image/stb_image.h"
)
target_include_directories(stb_image PUBLIC "${project_root}/3rdparty/stb_image")
set_target_properties(stb_image PROPERTIES FOLDER "3rdparty/stb_image")
