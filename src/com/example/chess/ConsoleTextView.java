package com.example.chess;

import java.util.LinkedList;
import java.util.ListIterator;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PorterDuffXfermode;
import android.graphics.PorterDuff;
import android.graphics.Typeface;
import android.graphics.Xfermode;
import android.util.AttributeSet;
import android.util.DisplayMetrics;
import android.widget.ImageView;
import android.graphics.Region.Op;

import com.example.chess.ConsoleScrollView.ScrollViewListener;
import com.example.chess.common.FontSize;

public class ConsoleTextView extends ImageView implements ScrollViewListener {

	private Paint textPaint;
	private LinkedList<String> textBufferList;
	private StringBuilder appendStringBuffer;

	private static final String NEWLINE = "\n";
	private static final int MAXLINE = 2048;
	private static final int DRAWYOFFSET = 16;

	private int BGCOLOR;
	private int FGCOLOR;
	private final int SCREENHEIGHT;
	
	private int LINEHEIGHT = 12;
	private int TEXTSIZE = 12;
	
	private int lineLimitInView;   ///< maximum number of lines which can be viewed by user 
	private int curViewYPos;       ///< current view Y position (value callback by scrollview)

	public ConsoleTextView(Context context) {
		this(context, null);
	}
	
	public ConsoleTextView(Context context, AttributeSet attrs) {
		this(context, attrs, 0);
	}
	
	public ConsoleTextView(Context context, AttributeSet attrs, int defStyle) {
	    super(context, attrs, defStyle);

	    DisplayMetrics metrics = context.getResources().getDisplayMetrics();	
	    SCREENHEIGHT = metrics.heightPixels;
	    
	    readAttributes(attrs);
	    
	    textPaint = new Paint();
	    
	    textPaint.setTypeface(Typeface.MONOSPACE);
	    textPaint.setColor(FGCOLOR);
	    setFontSize(TEXTSIZE);
	    
	    textBufferList = new LinkedList<String>();
	    appendStringBuffer = new StringBuilder(512);
	    
	    curViewYPos = 0;
	}
	
	public void clearAllText() {
		int removeCount = textBufferList.size();
		for (int i = 0; i < removeCount; ++i)
			textBufferList.remove(0);
		   
		   
		// redraw itself
		this.requestLayout();
		this.invalidate();
	}
	
	public void setFontSize(int fontSize) {
		
		FontSize fz = FontSize.toFontSize(fontSize, FontSize.SMALL);
		
		TEXTSIZE = fz.v();
		textPaint.setTextSize(TEXTSIZE);
		LINEHEIGHT = TEXTSIZE + 2;
		lineLimitInView =  (int)Math.ceil(SCREENHEIGHT / LINEHEIGHT);
	}
	
	public int getViewableHeight()  {
		return (DRAWYOFFSET + textBufferList.size() * LINEHEIGHT) - curViewYPos;
	}

	public int getFontSize() {
		return (int)textPaint.getTextSize();
	}	
	
	/**
	 * Add text to display in this view
	 * 
	 */
	  
	public void appendText(String s) { 
		   
		int index = 0;
		float maxWidth = this.getWidth();
		int searchStart = 0;
		   
		if(maxWidth == 0) return;  // ui hasn't been initialized yet so bail
		
		appendStringBuffer.delete(0, appendStringBuffer.length());
		   
		if (textBufferList.size() > 0) {
			// recalculate the last line
			appendStringBuffer.append(textBufferList.get(textBufferList.size() - 1));
			// remove the last line as we will recalculate
			textBufferList.remove(textBufferList.size() - 1);
		}

		appendStringBuffer.append(s);
	   
		while (index < appendStringBuffer.length()) {
			int lineBreakPos = appendStringBuffer.indexOf(NEWLINE, searchStart);
			int tempCount = 0; // num of char that will accomodate in a single line
			  
			if (lineBreakPos != -1) {
				// new line is found
				tempCount = textPaint.breakText(appendStringBuffer, index, lineBreakPos, 
													true, maxWidth, null);
				if (tempCount < (lineBreakPos - index))
					lineBreakPos = -1;   // linebreak not reached, leave for next line
			} else {
				tempCount = textPaint.breakText(appendStringBuffer, index, appendStringBuffer.length(), 
													true, maxWidth, null);
			}
			  
			  
			textBufferList.add(appendStringBuffer.substring(index, index + tempCount));
			index += (tempCount + ((lineBreakPos != -1) ? 1 : 0));
			searchStart = index;

			if (lineBreakPos != -1 && index == appendStringBuffer.length()) { // linebreak at last char
				textBufferList.add("");  // add one blank line to it
			}
		}
		   
		   
		// remove oldest line if too many
		if (textBufferList.size() > MAXLINE) {
			int removeCount = textBufferList.size() - MAXLINE;
			for (int i = 0; i < removeCount; ++i)
				textBufferList.remove(0);
		}
		   
		
		// redraw itself
		this.requestLayout();
		this.invalidate();
	}	
	
	@Override
	public void onViewScroll(int x, int y, int oldx, int oldy) {
		curViewYPos = y;
	}
	
	private Xfermode clearMode;
	private Paint clear;
	
	@Override
	protected void onDraw(Canvas canvas) {

		//background color
		canvas.clipRect(0, 0, getWidth(), getHeight(), Op.REPLACE);   
		canvas.drawColor(BGCOLOR);
		
		//canvas.clipRect(0, 0, getWidth(), getHeight(), Op.REPLACE);            
		//textPaint.setColor(BGCOLOR);
		//canvas.drawRect(0, 0, getWidth(), getHeight(), textPaint);		
		
		float YOffset = DRAWYOFFSET; // don't draw at pixel 0 as it can't be seen
		   
		int startLine = curViewYPos / LINEHEIGHT;
		
		startLine = (startLine >= textBufferList.size()) ? (textBufferList.size() - 1) : startLine;
		startLine = (startLine < 0) ? 0 : startLine;
		float posY = YOffset + startLine * LINEHEIGHT; // starting Y pos to draw
		
		ListIterator<String> itr = textBufferList.listIterator(startLine);
		int drawLineCount = 0;

		
		while (itr.hasNext()) {
			String s = itr.next();
			   
			canvas.drawText(s, 0, posY, textPaint);
			posY += LINEHEIGHT;

			++drawLineCount;
			if (drawLineCount > lineLimitInView)  // no need to draw if out of screen
				break;
		}
		
	}
	
	@Override
	protected void onMeasure (int widthMeasureSpec, int heightMeasureSpec) {
		int tempLineCount = textBufferList.size() < lineLimitInView ? lineLimitInView : textBufferList.size();
		int height = getDefaultSize(tempLineCount * LINEHEIGHT, heightMeasureSpec);
		int width = getDefaultSize(0, widthMeasureSpec);
		
		setMeasuredDimension(width, height);
	}
	
	private void readAttributes(AttributeSet attrs) {
		
	    int bgcolor = 0, fgcolor = 0xffffffff;
	    int textSize = 12;
	    
	    if (attrs!=null) {
	    	
	    	TypedArray a=getContext()
	    			.obtainStyledAttributes(attrs,
	    					R.styleable.ConsoleTextView,
	    					0, 0);
	        
	    	
	    	bgcolor = a.getInt(R.styleable.ConsoleTextView_backgroundColor,0xFF000000);
	    	fgcolor = a.getInt(R.styleable.ConsoleTextView_textColor, 0xFFFFFFFF);
	    	textSize = (int) a.getDimension(R.styleable.ConsoleTextView_textSize, 12);
	    	
	        a.recycle();
	    }
	    
	    BGCOLOR = bgcolor;
	    FGCOLOR = fgcolor;
	    TEXTSIZE = textSize;
	}

}
