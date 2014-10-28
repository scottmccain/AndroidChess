#include <jni.h>
#include <android/log.h>

#ifndef WRAPPER_INCLUDE
#define WRAPPER_INCLUDE

#include <zip.h>
extern struct zip* APKArchive;

#define STRINGIFY(x) #x
#define LOG_TAG    __FILE__ ":" STRINGIFY(__LINE__)
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#endif
