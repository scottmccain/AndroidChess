LOCAL_PATH := $(call my-dir)
SOURCE_PATH := $(LOCAL_PATH)/crafty
 
include $(CLEAR_VARS)
 
LOCAL_MODULE    			:= chess
LOCAL_STATIC_LIBRARIES 	:= libzip
LOCAL_CFLAGS 				:= -Wall -pipe -O3 -pthread -DUNIX -DSMP -DCPUS=2 -DEPD -DSKILL -DANDROID_NDK -Wno-psabi
LOCAL_C_INCLUDES 			:= $(LOCAL_PATH)/include/ $(SOURCE_PATH)/ $(LOCAL_PATH)/../libzip/
LOCAL_SRC_FILES 			:= crafty.c egtb.cpp wrapper.c buffer.c fifo_char.cpp util.cpp FifoQueue.cpp
LOCAL_LDLIBS 				:= -llog -lz


include $(BUILD_SHARED_LIBRARY)