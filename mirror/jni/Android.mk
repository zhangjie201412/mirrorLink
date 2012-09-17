LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE	:= mirror
LOCAL_SRC_FILES	:= mirror.c

LOCAL_LDLIBS	:= -llog

include $(BUILD_SHARED_LIBRARY)
