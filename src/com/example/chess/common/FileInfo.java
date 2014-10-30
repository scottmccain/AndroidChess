package com.example.chess.common;

import java.io.File;

import android.content.pm.ApplicationInfo;

public class FileInfo {
	
	private String apkFilePath;
	private String dataDirectory;
	private String cacheDirectory;
	
	public FileInfo(ApplicationInfo appInfo, File cacheDir) {
		
		apkFilePath = appInfo.sourceDir;
		dataDirectory = appInfo.dataDir;
		cacheDirectory = cacheDir.getAbsolutePath();
	}
	
	public String getApkFilePath() {
		return apkFilePath;
	}
	
	public String getDataDirectory() {
		return dataDirectory;
	}
	
	public String getCacheDirectory() {
		return cacheDirectory;
	}
	
}
