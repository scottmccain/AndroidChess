package com.example.jni;

import android.util.Log;

public class LibWrapper {
	
	private static String TAG = "LibWrapper";

	private static EventListener listener;
	
	public static interface EventListener {
		void OnMessage(String text);
	}

	public static void setListener(EventListener l) {
		listener = l;
	}
	
	public static native void SendMessage(String str);
	public static native void NativeInit(String apkDirectory, String cacheDirectory, String dataDirectory);
	
	private static void OnMessage(String text){
	    Log.e(TAG, "text:"+text);
	    
		if (listener != null) {
			listener.OnMessage(text);
		}	    
	 }
	
    static {
        System.loadLibrary("chess");
    }	
}
