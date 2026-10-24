add_library(AppSettings STATIC
    ArgsParser.cpp
    ArgsParser.h
    Settings.h
)

target_include_directories(AppSettings
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(AppSettings FlyCube)

set_target_properties(AppSettings PROPERTIES FOLDER "Modules")

if (BUILD_TESTING)
    add_executable(AppSettingsTest test/ArgsParserTest.cpp)
    target_link_libraries(AppSettingsTest PRIVATE Catch2WithMain AppSettings)
    set_target_properties(AppSettingsTest PROPERTIES FOLDER "Tests")
    add_test(NAME AppSettingsTest COMMAND AppSettingsTest)
endif()
