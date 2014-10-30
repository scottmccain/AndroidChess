package com.example.chess;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.ScrollView;

public class ConsoleScrollView extends ScrollView {

	private ScrollViewListener scrollViewListener = null;
	
	public ConsoleScrollView(Context context) {
		super(context);
	}
	
	public ConsoleScrollView(Context context, AttributeSet attrs, int defStyle) 
    {
        super(context, attrs, defStyle);
    }

    public ConsoleScrollView(Context context, AttributeSet attrs) 
    {
        super(context, attrs);
    }
    
    public void setScrollViewListener(ScrollViewListener scrollViewListener) 
    {
        this.scrollViewListener = scrollViewListener;
    }
    
    @Override
    protected void onScrollChanged(int l, int t, int oldl, int oldt)
    {
    	super.onScrollChanged(l, t, oldl, oldt);
 	    if (scrollViewListener != null)
 	    	scrollViewListener.onViewScroll(l, t, oldl, oldt);
    }
    
    public interface ScrollViewListener
	{
		void onViewScroll(int x, int y, int oldx, int oldy);
	}
	
}
