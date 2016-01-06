LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := libgles3jni
LOCAL_CFLAGS    := -Werror
LOCAL_SRC_FILES := main.cpp

LOCAL_LDLIBS    := -llog -lGLESv3 -lEGL -landroid

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../common/utils/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../common/testscene/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../common/mlogger/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../3rdparty/glm

include $(BUILD_SHARED_LIBRARY)