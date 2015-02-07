#pragma once

#if defined(__ANDROID__) || defined(ANDROID)
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#define LOG_TAG "FlyCube"
#include <mlogger_only.h>
#else
#include <gl_core.h>
#include <mlogger.h>
#endif
