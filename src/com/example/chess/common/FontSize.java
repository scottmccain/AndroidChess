package com.example.chess.common;

public enum FontSize
{
   UNKNOWN(-1),
   BIG(20),
   MEDIUM(16),
   SMALL(12);
   
   private final int size;   ///< actual font size
   FontSize(int s)
   {
      this.size = s;
   }
      
   public int v()
   {
      return this.size;
   }
   
   public static FontSize toFontSize(int size, FontSize def)
   {
	  for (FontSize f : FontSize.values()) {
		 if (f.v() == size)
			return f;
	  }
	  
	  return def;
   }
};