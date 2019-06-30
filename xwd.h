#ifndef xwd_h
#define xwd_h

/*  $Id: xwd.h,v 1.11 2019/03/01 18:25:16 dkl Exp $  */

#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include <X11/XWDFile.h>
#include <stdio.h>

#define MAXLN 512		// longest line of input text we allow

#define DEFAULT_FONT "6x9.bdf"
#define DEFAULT_FONT_DPI "misc"
#define DEFAULT_FONT_PATH "/usr/local/share/fonts"  // where we install bdfs
	

#define min(x,y) (((x) < (y)) ? (x) : (y))
#define max(x,y) (((x) > (y)) ? (x) : (y))

typedef 	unsigned long 	Pixel;
typedef 	unsigned short 	Color;

class xwd {
   public:
		xwd (const char *name);		// for use with images which have no DPI
		~xwd ();

		int 		MakeBW();			// make black-and-white
		int 		Use4();				// make 4-bit data
		int 		Use8();				// make 8-bit data
		int 		Resize (int, int);
		int 		Halve ();
		int 		Sample ();
		int 		Crop (int, int, int, int);
		int 		Fill (int, int, int, int);
		void 		Clear ();
		void 		Set ();
		void 		Trim();
		void 		TrimLeft();
		void 		TrimRight();
		void 		TrimTop();
		void 		TrimBottom();
		void 		DumpHeader ();
		void 		DumpImage ();
		void 		DumpLine (int line=0);
		void 		Dump();
		int 		Scale (double);
		int 		ReadXWD (int);
		int 		WriteXWD ();
		void 		DumpCmap(int cells=255);
		int 		SquishCmap();
		int 		SetWhitePixel(Pixel);
		int 		SetBlackPixel(Pixel);
		int 		SwapBW();
		int 		SetBackground(Pixel);
		int 		SetForeground(Pixel);
		int 		SetForeground(char *);
		int 		Uniq();
		int 		AddColor(char *);
		int 		AddColor(XColor *);
		int 		AddColor(int, int, int);
		int 		HasColor(XColor *);
		Pixel 	GetColor (int r, int g, int b);
		Pixel 	GetColor (char *);
		int 		ReadRGB (char *colorname, int *r, int *g, int *b);
		int 		FindRGBByName (char *);
		int 		NumRGBs ();
		int 		lsColors();
		void 		MergeCmap(xwd *intoMe);
		void 		MapTo(xwd *toMe);
		void 		DrawBorder (const char *borderName);
		int 		DrawLines (char *);
		void 		DrawLine (int, int, int, int);
		void 		DrawRect (int, int, int, int);
		unsigned	ImageSize ();
		void 		DrawString(char *text, int x, int y, int alignment);
		Pixel 	GetPixel (register int x, register int y);
		void 		PutPixel (register int x, register int y, Pixel pix);
	
		void 		SetName (char *);
		void 		SetPath (char *);
		void 		SetFont (char *);
		void 		SetFontPath (char *);
		int 		ShowFonts ();
		int 		ListPath (char *);
		void 		SetFontDPI (char *);
		void 		SetPenWidth (unsigned long setting) {penWidth=setting;}
		int 		GetClosestColor (int r, int g, int b);
		int 		GetFarthestColor (int r, int g, int b);
	
		void 				SetVerbose (Bool setting) {verbose = setting;};
		void 				Error (const char *);
	
		char 				*name, *path, *indx, *font, *fontPath, *fontDPI;
		XImage 			*image;
		XWDFileHeader 	*header;
		XColor 			*colors;
		int 				ncolors;
		Bool 				debug, verbose, writeGzipped;
		unsigned long	whitePixel, blackPixel, fg, bg, penWidth;
	
	private:
		Bool 		Read (char *ptr, int size, int nitems, FILE *stream);
		void 		DrawSteepLine (int, int, int, int);
		void 		DrawShallowLine (int, int, int, int);
		
	static void 	_swapshort (char *, unsigned);
	static void 	_swaplong (char *, unsigned);
};
	
#define sameColor(c1, c2) ((c1.red == c2.red) && \
			    (c1.green == c2.green) && \
			    (c1.blue == c2.blue))

#endif /* xwd_h */
