package com.example.chess;

import com.example.chess.common.FileInfo;
import com.example.jni.LibWrapper;

import android.support.v7.app.ActionBarActivity;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.Display;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.TextView;

public class MainActivity extends ActionBarActivity implements com.example.jni.LibWrapper.EventListener {

    //private TextView txtMessage;
    private EditText sendMessageText;

	private ConsoleScrollView scrollView;
	private ConsoleTextView outputTextView;
	
    private LibWrapper _nativeWrapper;
    
    private boolean mStarted = false;
    
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		
		Initialize();
		
		//txtMessage = (TextView) findViewById(R.id.lblFromNative);
		sendMessageText = (EditText) findViewById(R.id.inputBox);
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
	public void OnMessage(final String text) {
		runOnUiThread(new Runnable() {
            @Override
            public void run() {
            	outputTextView.appendText(text);
            }
        });
	}
	
    public void cbSend(View view)
    {
        String value = sendMessageText.getText().toString();
        LibWrapper.SendMessage(value + "\n");
        sendMessageText.setText("");
    }	
    
    private View.OnClickListener butClickListener = new View.OnClickListener() 
    {
    	public void onClick(View v) {
    		cbSend(v);
		}
    	
    };
    
    private void Initialize() {
   	
    	outputTextView = (ConsoleTextView)findViewById(R.id.textView);
    	
    	scrollView = (ConsoleScrollView)findViewById(R.id.consoleScrollView);
    	scrollView.setScrollViewListener(outputTextView);
    	
    	// init input button
    	ImageButton inputOK = (ImageButton) findViewById(R.id.inputOKButton);
    	inputOK.setOnClickListener(butClickListener);
    	
		ApplicationInfo appInfo = null;
		PackageManager packMgmr = getPackageManager();
		try {
			appInfo = packMgmr.getApplicationInfo("com.example.chess", 0);
		} catch (NameNotFoundException e) {
			e.printStackTrace();
			throw new RuntimeException("Unable to locate assets, aborting...");
		}
    	
		FileInfo fi = new FileInfo(appInfo, getCacheDir());

    	InitializeNative(fi);
    	
    	final String [] argv = {"logpath=" + fi.getDataDirectory()};
    	
        new Thread(new Runnable() {
			public void run() {
				mStarted = true;
				LibWrapper.ChessMain(argv);
			}
		}).start();    	
    }
    
    private void InitializeNative(FileInfo fi) {
		LibWrapper.setListener(this);
		
		
		//apkFilePath = appInfo.sourceDir;
		//dataDirectory = appInfo.dataDir;
		//cacheDirectory = getCacheDir().getAbsolutePath();
		
		LibWrapper.NativeInit(fi.getApkFilePath(), fi.getCacheDirectory(), fi.getDataDirectory());
    }
}
