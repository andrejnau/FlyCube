#pragma once

#if defined(__ANDROID__) || defined(ANDROID)
    #include <EGL/egl.h>
    #include <GLES3/gl3.h>
    #define LOG_TAG "FlyCube"
#else
    #include <gl_core_4_5.h>
#endif

#include <mlogger.h>