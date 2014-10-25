#include "wrapper.h"

static jmethodID mSendStr;
static jclass jNativesCls;
static JavaVM *g_VM;
static int initialized = 0;

#define CLASS_NAME "com/example/jni/LibWrapper"
#define CALLBACK_METHOD_NAME "Callback"

/*
 * Output to java layer
 */
static void jni_send_str(const char * text)
{
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

		if (!mSendStr) {
			mSendStr = (*env)->GetStaticMethodID(env, jNativesCls
					, CALLBACK_METHOD_NAME
					, "(Ljava/lang/String;)V");
		}

		if(mSendStr) {
			(*env)->CallStaticVoidMethod(env, jNativesCls
					, mSendStr
					, (*env)->NewStringUTF(env, text) );
		}
	}
}


void jni_printf(const char *format, va_list argptr)
{
	static char string[1024];
	vsnprintf (string, 1023, format, argptr);
	jni_send_str(string);
}

void buffer_send(const char*fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	jni_printf(fmt, ap);
	va_end(ap);
}

/*
 * Input from java layer
 */
JNIEXPORT void JNICALL Java_com_example_jni_LibWrapper_SendBuffer
  (JNIEnv * je, jclass jc, jstring js)
{
	// put data in buffer
	// raise flag that buffer is ready to read

	// TODO:
	// provide read functions for buffer
	// that will reset flag and send contents of buffer
	// use spin locks / mutex for thread safety
}

/*
 * Initialize native layer
 */
JNIEXPORT void JNICALL Java_com_example_jni_LibWrapper_Init
  (JNIEnv * env, jclass jc)
{
	static int initialized = 0;

	if(!initialized)
	{
		(*env)->GetJavaVM(env, &g_VM);
		initialized = 1;
	}
}
