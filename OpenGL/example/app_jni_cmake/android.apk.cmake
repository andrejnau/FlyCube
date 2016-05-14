#*********************************************************#
#*  File: Apk.cmake                                      *
#*    Android apk tools
#*
#*  Copyright (C) 2002-2013 The PixelLight Team (http://www.pixellight.org/)
#*  Copyright (C) 2016 softmob
#*
#*  This file is part of PixelLight.
#*
#*  Permission is hereby granted, free of charge, to any person obtaining a copy of this software
#*  and associated documentation files (the "Software"), to deal in the Software without
#*  restriction, including without limitation the rights to use, copy, modify, merge, publish,
#*  distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
#*  Software is furnished to do so, subject to the following conditions:
#*
#*  The above copyright notice and this permission notice shall be included in all copies or
#*  substantial portions of the Software.
#*
#*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
#*  BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
#*  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
#*  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#*  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#*********************************************************#



##################################################
## MACRO: android_create_apk
##
## @param name
##   Name of the project (e.g. "MyProject"), this will also be the name of the created apk file
## @param apk_package_name
##
## @param apk_directory
##
## @param shared_libraries
##   List of shared libraries (absolute filenames) this application is using, these libraries are copied into the apk file and will be loaded automatically within a generated Java file - Lookout! The order is important due to shared library dependencies!
## @param apk_output_directory
##  Output directory
##
## @remarks
##   Requires the following tools to be found automatically
##   - "android" (part of the Android SDK)
##   - "adb" (part of the Android SDK)
##   - "ant" (type e.g. "sudo apt-get install ant" on your Linux system to install Ant)
##################################################
macro(android_create_apk name apk_package_name apk_directory shared_libraries apk_output_directory)
    set(ANDROID_APK_PACKAGE ${apk_package_name})
    set(ANDROID_NAME ${name})

    add_custom_command(TARGET ${ANDROID_NAME}
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${apk_output_directory}
    )

    add_custom_command(TARGET ${ANDROID_NAME}
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${apk_directory}/assets ${apk_output_directory}/assets
    )

    add_custom_command(TARGET ${ANDROID_NAME}
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${apk_directory}/java ${apk_output_directory}/src
    )

    add_custom_command(TARGET ${ANDROID_NAME}
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${apk_directory}/res ${apk_output_directory}/res
    )

    add_custom_command(TARGET ${ANDROID_NAME}
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${apk_directory}/AndroidManifest.xml ${apk_output_directory}/AndroidManifest.xml
    )

    add_custom_command(TARGET ${ANDROID_NAME}
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${apk_output_directory}/libs/${ANDROID_ABI}"
    )

    # Copy the used shared libraries
    foreach(value ${shared_libraries})
        add_custom_command(TARGET ${ANDROID_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${value} "${apk_output_directory}/libs/${ANDROID_ABI}"
        )
    endforeach()

    # Create "build.xml", "default.properties", "local.properties" and "proguard.cfg" files
    add_custom_command(TARGET ${ANDROID_NAME}
        COMMAND android update project -t android-${ANDROID_API_LEVEL} --name ${ANDROID_NAME} --path ${apk_output_directory}
    )

    # Build the apk file
    add_custom_command(TARGET ${ANDROID_NAME}
            COMMAND ant debug
            WORKING_DIRECTORY "${apk_output_directory}"
    )
endmacro()