LOCAL_PATH := $(call my-dir)
SOURCE_PATH := $(LOCAL_PATH)/src
 
include $(CLEAR_VARS)
 
LOCAL_CFLAGS 		:= -DUNIX -DCPUS=2
LOCAL_MODULE    	:= chess
LOCAL_C_INCLUDES 	:= $(LOCAL_PATH)/include $(SOURCE_PATH)
LOCAL_SRC_FILES 	:= crafty.c src/egtb.cpp
 
include $(BUILD_SHARED_LIBRARY)