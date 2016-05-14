cmake_minimum_required (VERSION 3.0)

## cmake.exe -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE="../../3rdparty/android-cmake-master/android.toolchain.cmake" -DANDROID_ABI="armeabi-v7a" -DANDROID_API_LEVEL=23 -DANDROID_STL=c++_shared ..

set (PROJECT gles3jni)
project (${PROJECT})

include("android.apk.cmake")

include_directories("../../common/utils/include"
                    "../../common/testscene/include"
                    "../../common/mlogger/include"
                    "../../3rdparty/glm"
                    ${ANDROID_SYSROOT}/usr/include
                    ${ANDROID_STL_INCLUDE_DIRS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Werror -fno-rtti -fno-exceptions")
set(LINKER_FLAGS "${LINKER_FLAGS} --Wl,-rpath-link=${ANDROID_NDK_SYSROOT}/usr/lib -L${ANDROID_NDK_SYSROOT}/usr/lib")

set(SOURCE ../app_jni/app/src/main/jni/main.cpp)

add_library(${PROJECT} SHARED ${SOURCE})
target_link_libraries(${PROJECT} android log GLESv3 EGL)

set(APK_PACKAGE_NAME "com.softmob.gles3jni")
set(APK_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../app_jni/app/src/main")
set(APK_SHARED_LIBRARIES ${CMAKE_CURRENT_BINARY_DIR}/libgles3jni.so ${__libstl})
set(APK_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/apk)

android_create_apk(${PROJECT}
                   ${APK_PACKAGE_NAME}
                   "${APK_DIRECTORY}"
                   "${APK_SHARED_LIBRARIES}"
                   "${APK_OUTPUT_DIRECTORY}")