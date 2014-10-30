#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "wrapper_jni.h"
#include "logging.h"
//#include "lock.h"
//#include "fifo_char.h"
#include "util.h"

#define CLASS_NAME "com/example/jni/LibWrapper"
#define CALLBACK_METHOD_NAME "OnMessage"
//#define FIFO_SIZE 4096

extern int chess_main(int argc, char **argv);
extern int read_buffer(char * pz, int size);
extern void initalize_buffer();
extern void buffer_write_string(char *sz);
extern int buffer_size();
extern void loadAPK (const char* apkPath);

//extern lock_t lock_buffer;
//extern struct zip* APKArchive;

static jmethodID mSendStr;
static jclass jNativesCls;
static JavaVM *g_VM;

static char _dataDirectory[512];
static char _cacheDirectory[512];


//static charfifo *pfifo;

const int getArrayLen(JNIEnv * env, jobjectArray jarray) {
	return (*env)->GetArrayLength(env, jarray);
}

/*
 * Output to java layer
 */
void jni_send_str(const char * text) {
	JNIEnv *env;

	if (!g_VM) {
		return;
	}

	(*g_VM)->AttachCurrentThread(g_VM, &env, 0);

	if (!jNativesCls) {
		jNativesCls = (*env)->FindClass(env, CLASS_NAME);
		if (jNativesCls == 0) {
			return;
		}
	}

	if (!mSendStr) {
		mSendStr = (*env)->GetStaticMethodID(env, jNativesCls
				, CALLBACK_METHOD_NAME
				, "(Ljava/lang/String;)V");

		if(mSendStr == 0)
			return;
	}

	jstring jstext = (*env)->NewStringUTF(env, text);
	(*env)->CallStaticVoidMethod(env, jNativesCls
			, mSendStr
			, jstext );

	(*env)->DeleteLocalRef(env, jstext);

}

void make_path(const char *szDirectory, const char *szFilename, char * rcdestination, size_t size) {

	strlcpy(rcdestination, szDirectory, size);
	strlcat(rcdestination, szFilename, size);
}

int file_exists(const char *szPath) {
	struct stat sb;

	if (stat(szPath, &sb) == -1) {
		return 0;
	}

	return 1;
}

void unpack_files() {

	// open manifest file and read each file
	// check if file exists in data directory
	// extract file if it doesn't exist

	// 1. unpack .rc file and book files
	// 2. unpack and extract table database
	// 3. create version file and write current version

	char * rcfilename = "assets/data/crafty.rc";
	char rcdestination[512];
	//int fd;

	make_path(_dataDirectory, "/crafty.rc", rcdestination, sizeof(rcdestination));
	LOGI("extracting %s to %s", rcfilename, rcdestination);

	if(extract_file(rcfilename, rcdestination) == 0) {
		if(file_exists(rcdestination)) {
		//fd = open(rcdestination, O_RDONLY, 0);
		//if(fd > 0) {
			// check file
			LOGI("extract successful!");
		}
		//close(fd);
	}
}

/* wrapper API */
void jni_printf(const char *format, va_list argptr) {
	static char string[1024];
	vsnprintf (string, 1023, format, argptr);
	jni_send_str(string);
}

void native_send(const char*fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	jni_printf(fmt, ap);
	va_end(ap);
}



void native_sound(const char* sound_name) {
}

/*
 * Input from java layer
 */
JNIEXPORT void JNICALL Java_com_example_jni_LibWrapper_SendMessage
  (JNIEnv * env, jclass jc, jstring js) {

	const char *nativeString = (*env)->GetStringUTFChars(env, js, 0);

	buffer_write_string((char *)nativeString);

	//Lock(lock_buffer);
	//fifo_char_write((void **)&pfifo, nativeString, strlen(nativeString));
	//Unlock(lock_buffer);

	LOGI("Received string: %s, buffer size is now: %d", nativeString, buffer_size());
	native_send("Received string: %s\nBuffer size is now: %d\n", nativeString, buffer_size());

	(*env)->ReleaseStringUTFChars(env, js, nativeString);
}


jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env;
    g_VM = vm;

    if ((*g_VM)->GetEnv(g_VM, (void **)&env, JNI_VERSION_1_2) != JNI_OK) {
        return -1;
    }

    jclass localClass = (*env)->FindClass(env, CLASS_NAME);
    jNativesCls = (jclass)((*env)->NewGlobalRef(env, localClass));
	if (jNativesCls == 0) {
		return -1;
	}

	mSendStr = (*env)->GetStaticMethodID(env, jNativesCls
			, CALLBACK_METHOD_NAME
			, "(Ljava/lang/String;)V");

	initalize_buffer();

	//fifo_char_create((void **)&pfifo, FIFO_SIZE + 1);
	//LOGI("Fifo queue initialized with size %d", FIFO_SIZE);

    return JNI_VERSION_1_2;
}

/*
 * Initialize native layer
 */
JNIEXPORT void JNICALL Java_com_example_jni_LibWrapper_NativeInit
  (JNIEnv * env, jclass jc, jstring apkPath, jstring cache, jstring data)
{
	static int initialized = 0;

	if(!initialized) {
		const char* str;
		str = (*env)->GetStringUTFChars(env, apkPath, 0);
		loadAPK(str);

		initialized = 1;
		native_send("Initialization complete!  Apk Directory: %s\r\n", str, 4096);

		(*env)->ReleaseStringUTFChars(env, apkPath, str);

		str = (*env)->GetStringUTFChars(env, cache, 0);
		strlcpy(_cacheDirectory, str, sizeof(_cacheDirectory));
		(*env)->ReleaseStringUTFChars(env, cache, str);

		str = (*env)->GetStringUTFChars(env, data, 0);
		strlcpy(_dataDirectory, str, sizeof(_dataDirectory));
		(*env)->ReleaseStringUTFChars(env, data, str);

		LOGI("Data Directory : %s\nCache Directory : %s\n", _dataDirectory, _cacheDirectory);
		unpack_files();
	}
}

/*
 * Class:     quake_jni_Natives
 * Method:    QuakeMain
 * Signature: ([Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_example_jni_LibWrapper_ChessMain
  (JNIEnv * env, jclass cls, jobjectArray jargv)
{
	(*env)->GetJavaVM(env, &g_VM);

	// Extract char ** args from Java array
	jsize clen =  getArrayLen(env, jargv);

	char * args[(int)clen];

	int i;
	jstring jrow;
	for (i = 0; i < clen; i++)
	{
	    jrow = (jstring)(*env)->GetObjectArrayElement(env, jargv, i);
	    const char *row  = (*env)->GetStringUTFChars(env, jrow, 0);

	    args[i] = malloc( strlen(row) + 1);
	    strcpy (args[i], row);

	    // free java string jrow
	    (*env)->ReleaseStringUTFChars(env, jrow, row);
	}

	for (i = 0; i < clen; i++) {
		native_send("ChessMain args[%d]=%s", clen, args[i]);
	}

	// invoke main, program will loop forever
	chess_main(clen, args);

	return 0;
}
