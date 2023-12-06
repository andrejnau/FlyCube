list(APPEND headers
   AppBox.h
   AppDelegate.h
   AppView.h
   AppViewController.h
   AppViewDelegate.h
)

list(APPEND sources
   AppBox.mm
   AppDelegate.mm
   AppView.mm
   AppViewController.mm
)

add_library(AppBoxUIKit OBJECT
   ${headers}
   ${sources}
)

target_include_directories(AppBoxUIKit
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(AppBoxUIKit
   Instance
   "-framework Metal"
   "-framework QuartzCore"
   "-framework UIKit"
)

set_target_properties(AppBoxUIKit PROPERTIES FOLDER "Modules")
