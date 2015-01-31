#pragma once

#ifdef __ANDROID__
#include <android/log.h>
#include <jni.h>

#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include <sys\types.h>
#include <android\asset_manager.h>
#include <android\asset_manager_jni.h>

#define LOG_TAG "GLES3JNI"
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#else
#include <gl_core.h>
#include <stdio.h>
#define ALOGE(...) printf(__VA_ARGS__)
#define ALOGV(...) printf(__VA_ARGS__)
#endif

