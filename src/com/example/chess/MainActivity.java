package com.example.chess;

import com.example.jni.LibWrapper;

import android.support.v7.app.ActionBarActivity;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.TextView;

public class MainActivity extends ActionBarActivity implements com.example.jni.LibWrapper.EventListener {

    private TextView txtMessage;

    private LibWrapper _nativeWrapper;
    
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		
		txtMessage = (TextView) findViewById(R.id.lblFromNative);

		LibWrapper.setListener(this);
		
		String apkFilePath = null;
		String cacheDirectory = null;
		String dataDirectory = null;
		ApplicationInfo appInfo = null;
		PackageManager packMgmr = getPackageManager();
		try {
			appInfo = packMgmr.getApplicationInfo("com.example.chess", 0);
		} catch (NameNotFoundException e) {
			e.printStackTrace();
			throw new RuntimeException("Unable to locate assets, aborting...");
		}
		apkFilePath = appInfo.sourceDir;
		dataDirectory = appInfo.dataDir;
		cacheDirectory = getCacheDir().getAbsolutePath();
		
		LibWrapper.NativeInit(apkFilePath, cacheDirectory, dataDirectory);
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		// Handle action bar item clicks here. The action bar will
		// automatically handle clicks on the Home/Up button, so long
		// as you specify a parent activity in AndroidManifest.xml.
		int id = item.getItemId();
		if (id == R.id.action_settings) {
			return true;
		}
		return super.onOptionsItemSelected(item);
	}

	@Override
	public void OnMessage(String text) {
		txtMessage.setText(text);
	}
	
    public void cbSend(View view)
    {
        String value = txtMessage.getText().toString();
        LibWrapper.SendMessage(value + "/r/n");
    }	
}
