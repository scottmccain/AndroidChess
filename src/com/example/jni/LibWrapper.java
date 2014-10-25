package com.example.jni;

import android.util.Log;

public class LibWrapper {
	private static String TAG = "LibWrapper";
	
	public static native void SendBuffer(String str);
	public static native void Init();
	
	public static void Callback(String value){
	    Log.e(TAG, "value:"+value);
	 }
}
