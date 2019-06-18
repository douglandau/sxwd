    
/* $Id: sxwd.cc,v 1.13 2019/03/01 18:23:43 dkl Exp $ */ 

/* sxwd - client to drive xwd.cc */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "xwd.h"
#include "ppm.h"
#include "font.h"

int 		new_width, new_height;
Bool 		debug=False, verbose=False;
char 		*progname;

/*  forward declarations  */

void usage(const char *msg) {
    fprintf (stderr, "\n%s\n\n", msg);
    fprintf (stderr, "usage:  %s [option [| option]] -in <filename> [-out <filename>]\n",progname);
    fprintf(stderr, " Where options are:\n");
    fprintf(stderr, "           [-bg <pixel>] \n");
    fprintf(stderr, "           [-blackPixel] \n");
    fprintf(stderr, "           [-border] \n");
    fprintf(stderr, "           [-bw] \n");
    fprintf(stderr, "           [-c16 \n");
    fprintf(stderr, "           [-clear \n");
    fprintf(stderr, "           [-clip|-crop] \n");
    fprintf(stderr, "           [-cmap] \n");
    fprintf(stderr, "           [-debug] \n");
    fprintf(stderr, "           [-dpi <DPI>] \n");
    fprintf(stderr, "           [-dump] \n");
    fprintf(stderr, "           [-dumpLine] \n");
    fprintf(stderr, "           [-get] \n");
    fprintf(stderr, "           [-font] \n");
    fprintf(stderr, "           [-fontPath <path>] \n");
    fprintf(stderr, "           [-fg <pixel>] \n");
    fprintf(stderr, "           [-gz] \n");
    fprintf(stderr, "           [-help] \n");
    fprintf(stderr, "           [-image] \n");
    fprintf(stderr, "           [-header] \n");
    fprintf(stderr, "           [-image] \n");
    fprintf(stderr, "           [-info] \n");
    fprintf(stderr, "           [-in] <filename>\n");
    fprintf(stderr, "           [-line x1 y1 x2 y2]\n");
    fprintf(stderr, "           [-lscolors]\n");
    fprintf(stderr, "           [-lsfonts]\n");
    fprintf(stderr, "           [-map] <filename>\n");
    fprintf(stderr, "           [-out] <filename>\n");
    fprintf(stderr, "           [-over15] \n");
    fprintf(stderr, "           [-name] <filename>\n");
    fprintf(stderr, "           [-path] <path>\n");
    fprintf(stderr, "           [-patch] <filename>\n");
    fprintf(stderr, "           [-put pixel r g b] \n");
    fprintf(stderr, "           [-rect x1 y1 x2 y2]\n");
    fprintf(stderr, "           [-sample]\n");
    fprintf(stderr, "           [-set] \n");
    fprintf(stderr, "           [-shrink] \n");
    fprintf(stderr, "           [-squish] \n");
    fprintf(stderr, "           [-swapBW] \n");
    fprintf(stderr, "           [-text sometext] \n");
    fprintf(stderr, "           [-trim] \n");
    fprintf(stderr, "           [-use4] \n");
    fprintf(stderr, "           [-use8] \n");
    fprintf(stderr, "           [-verbose] \n");
    fprintf(stderr, "           [-whitePixel] \n");
    fprintf(stderr, "           [-x] \n");
    fprintf(stderr, "           [-x1] \n");
    fprintf(stderr, "           [-x2] \n");
    fprintf(stderr, "           [-y] \n");
    fprintf(stderr, "           [-y2] \n");
    fprintf(stderr, "           [-y2] \n");
    exit(1);
}

int main (int argc, char **argv) {
    int 		i, j, n, r, c, x=0, y=0;
    int 		x1, y1, x2, y2;
    unsigned 	buffer_size;
    int 		win_name_size=0, numImages=0;
    char 	*fileName=NULL, *outName=NULL, *mapName=NULL, *font=NULL;
    char 	*patchName=NULL, *patchCenteredName=NULL, *bustout=NULL;
    int 		cell=-1, trunc=-1;
    char 	*win_name=NULL, *text=NULL, *fontPath=NULL, *fontDPI=NULL;
    XWDColor 	xwdcolor;
    xwd 		*in, *map;
    Bool		shrink=False, crop=False,bbox=False,breakdown=False,c16=False,
            trim=False, squish=False, use4=False, use8=False, cmap=False;
    Bool		writeGzipped=False, info=False, check=False, corners=False;
    Bool		halve=False, sample=False, dump=False, dumpLine=False,swapBW=False;
    Bool		over15=False, uniq=False, get=False, set=False,clear=False,bw=False;
    Bool    dumpHeader=False, dumpImage=False, write=False, line=False;
    Bool    rect=False, fill=False, resize=False, header=False, image=False; 
    Bool    put=False, showFontPath=False, lsfonts=False, lsColors=False;
    char  	*border=NULL, *newName=NULL, *newPath=NULL, *addColor=NULL;
    char  	*family=NULL, *pt=NULL, *style=NULL, *dpi=NULL;
    char  	*fg=NULL, *bg=NULL;
    int		wp=-1, bp=-1, alignment=LEFT, w, h;
    int 		newred=-1, newgreen=-1, newblue=-1, ncolors=-1;
    double	scale=1.0;

	progname = argv[0];
	for (i = 1; i < argc; i++) {
		if (strcasecmp(argv[i], "-addColor") == 0) {
		    if (++i >= argc) usage("");
		    addColor = argv[i];
	    	in->AddColor(addColor); 
		    continue;
		}
		if (strcasecmp(argv[i], "-alignment") == 0) { 
		    if (++i >= argc) usage("");
		    if (strcasecmp(argv[i],"LEFT")==0) alignment = LEFT;
		    if (strcasecmp(argv[i],"left")==0) alignment = LEFT;
		    if (strcasecmp(argv[i],"CENTER")==0) alignment = CENTER;
		    if (strcasecmp(argv[i],"center")==0) alignment = CENTER;
		    if (strcasecmp(argv[i],"RIGHT")==0) alignment = RIGHT;
		    if (strcasecmp(argv[i],"right")==0) alignment = RIGHT;
		    continue;
		}
		if (strcasecmp(argv[i], "-bbox") == 0) { bbox = True; continue; }
		if (strcasecmp(argv[i], "-bg") == 0) { 
		   if (++i >= argc) usage("");
		   bg = argv[i]; 	
			if (bg) { 
				if (strcasecmp(bg,"white")==0) in->SetBackground(in->whitePixel);
				else if (strcasecmp(bg,"black")==0) in->SetBackground(in->blackPixel);
				else in->SetBackground(atoi(bg));
			}
			continue;
		}
		if (strcasecmp(argv[i], "-border") == 0) { 
		   if (++i >= argc) usage("");
		   border = argv[i]; 		
			continue;
		}
		if (strcasecmp(argv[i], "-breakdown") == 0) { 
			breakdown = True; 
			continue;
		}
		if (strcasecmp(argv[i], "-bw") == 0) { 
			bw = True; 	
	    	in->MakeBW();
			continue;
		}
		if ((strcasecmp(argv[i], "-clip") == 0) || 
	  		 (strcasecmp(argv[i], "-crop")) == 0) { 
			crop = True;
		   x1 = atoi(argv[++i]); y1 = atoi(argv[++i]); 	
			x2 = atoi(argv[++i]); y2 = atoi(argv[++i]); 	
	   	in->Crop (x1,y1,x2,y2); 
			continue;
		}
		if ((strcasecmp(argv[i], "-check") == 0) ||
	  		 (strcasecmp(argv[i], "-info")) == 0) { 
			check = True;
	      int count = 0, siz = in->image->width * in->image->height;
	      printf ("wxh: %dx%d, ncolors: %d, header->ncolors: %d, unique colors: %d\n", 
				in->image->width, in->image->height, in->ncolors, in->header->ncolors, in->Uniq()); 
	      printf ("whitePixel: %d, blackPixel: %d\n", 
					(int)in->whitePixel, (int)in->blackPixel);
	      for (j = 0; j < siz; j++) 
	        if ((unsigned char)in->image->data[j] > in->header->ncolors)
		 		printf ("pixel value of point %d greater than number of colors (%d)\n",
							j,in->image->data[j]);
	      for (j = 0; j < siz; j++) 
				if ((unsigned char)in->image->data[j] > 15)
		    		count++;
	      printf("found %d pixels over 15\n", count);
	      printf("fontPath: %s\n", in->fontPath);
	      printf("fontDPI: %s\n", in->fontDPI);
	      printf("font: %s\n", in->font);
			continue;
		}
		if (strcasecmp(argv[i], "-clear") == 0) { 
			clear = True; 		
	    if (clear) in->Clear();
			continue;
		}
		if (strcasecmp(argv[i], "-cmap") == 0) { 
			cmap = True; 		
	    	in->DumpCmap(255); 
			continue;
	   }
		if (strcasecmp(argv[i], "-c16") == 0) { 
			c16 = True; 		
	    	in->DumpCmap(16); 
			continue;
		}
		if (strcasecmp(argv[i], "-debug") == 0) { 
			debug = True; 		
	   	in->debug = 1;
			continue;
		}
		if (strcasecmp(argv[i], "-dump") == 0) { 
			dump = True; 		
	    	in->Dump(); 
			continue;
		}
		if (strcasecmp(argv[i], "-dumpLine") == 0) { 
			dumpLine = True; 		
	    	in->DumpLine(0); 
			continue;
		}
		if (strcasecmp(argv[i], "-dpi") == 0) {
		   if (++i >= argc) usage("DPI vsluje expected");
		   dpi = argv[i]; 	
	    	in->SetFontDPI (dpi); 
			continue;
	   }
		if (strcasecmp(argv[i], "-fg") == 0) { 
		   if (++i >= argc) usage("Pixel value or 'black' or 'white' expected");
		   fg = argv[i]; 		
   		if (fg) { 
				if (strcasecmp(fg,"white")==0) in->SetForeground(in->whitePixel);
				else if (strcasecmp(fg,"black")==0) in->SetForeground(in->blackPixel);
				else {
					// is it a color name or a pixel number ?
	  				if ((fg[0]>='0') && (fg[0]<='9'))
						// it's a pixel value
	    				in->SetForeground(atoi(fg));
	  				else if ((fg[0]>='a') && (fg[0]<='z'))
						// it's a color name
	    				in->SetForeground(fg);
      		}
   		}
			continue;
		}
		if (strcasecmp(argv[i], "-family") == 0) {
		   if (++i >= argc) usage("");
		   family = argv[i]; 		
	      char *newfont = new char [256];
			sprintf (newfont, "%s%s%s.bdf", family, style, pt);
	    	in->SetFont(newfont);
			continue;
		}
		if (strcasecmp(argv[i], "-fill") == 0) {
		   if (i+4 >= argc) usage("");
		   fill = True;   x1 = atoi(argv[++i]);
		   y1 = atoi(argv[++i]); 	x2 = atoi(argv[++i]);
		   y2 = atoi(argv[++i]); 		
	   	in->Fill (x1,y1,x2,y2); 
			continue;
		}
		if (strcasecmp(argv[i], "-font") == 0) {
	    	if (++i >= argc) usage("");
	    	font = argv[i]; 
			if (strstr(font,"bdf")) { 
		   	in->SetFont(font); 
			} else {
		   	char *s = new char(strlen(font)+8);
		   	sprintf (s, "%s.bdf", font);
		   	in->SetFont(s); 
			}
			continue;
		}
		if (strcasecmp(argv[i], "-fontDPI") == 0) {
		   if (++i >= argc) usage("");
		   font = argv[i]; 	
			continue;
		}
		if (strcasecmp(argv[i], "-fontPath") == 0) {
		   if (++i >= argc) usage("");
		   fontPath = argv[i]; 	
	     	in->SetFontPath (fontPath); 
			continue;
		}
		if (strcasecmp(argv[i], "-showFontPath") == 0) {
		    showFontPath = True; 	
			continue;
		}
		if (strcasecmp(argv[i], "-get") == 0) { 
		    if (++i >= argc) usage("");
		    get=True;   cell = atoi(argv[i]); 	
			printf("Cell %d: %d,%d,%d\n", cell, in->colors[(Pixel)cell].red,
		    	in->colors[(Pixel)cell].green, in->colors[(Pixel)cell].blue);
			continue;
		}
		if (strcasecmp(argv[i], "-halve") == 0) {
		    halve = True; 			
	   	//  resize it in halve by taking the darkest of each 4 pixels
	   	in->Halve(); 
			continue;
		}
		if (strcasecmp(argv[i], "-header") == 0) {
		    header = True; 		
	    	in->DumpHeader(); 
			continue;
		}
		if (strcasecmp(argv[i], "-help") == 0) { 
			usage("");
			continue;
		}
		if (strcasecmp(argv[i], "-image") == 0) {
		    image = True; 		
	    	in->DumpImage(); 
			continue;
		}
		if (strcasecmp(argv[i], "-in") == 0) {
		   if (++i >= argc) usage("");
		   fileName = argv[i]; 
	   	in = new xwd (fileName);
	   	//  read the image
	   	if (in->ReadXWD(1)) { 
				fprintf (stderr, "Error:  Can't read %s\n", fileName);
				exit(1);
	   	}
			continue;
		}
		if (strcasecmp(argv[i], "-info") == 0) {
		    info = True; 	
			continue;
		}
		if (strcasecmp(argv[i], "-line") == 0) {
		    line = True; 
		    if (i+4 > argc) usage("Not enough arguments to -line");
		    line = True;   x1 = atoi(argv[++i]);
		    y1 = atoi(argv[++i]); 	x2 = atoi(argv[++i]);
		    y2 = atoi(argv[++i]); 
	   	in->DrawLine(x1, y1, x2, y2); 
			continue;
		}
		if ((strcasecmp(argv[i], "-lsrgb") == 0) ||
		    (strcasecmp(argv[i], "-lscolors")) == 0) {
		    lsColors = True; 
			in->lsColors();
			continue;
		}
		if ((strcasecmp(argv[i], "-lsfonts") == 0) ||
		    (strcasecmp(argv[i], "-listFonts") ==0) ||
		    (strcasecmp(argv[i], "-showFonts")) == 0) {
		    lsfonts = True; 
			in->ShowFonts();
			continue;
		}
		if (strcasecmp(argv[i], "-map") == 0) {
		    if (++i >= argc) usage("");
		    mapName = argv[i]; 
	
	   	//  adopt and map to another xwd's colormap
	      map = new xwd (mapName);
	      //  read the image
	      if (map->ReadXWD(1)) { 
		      fprintf (stderr, "Error:  Can't read %s\n", mapName);
		      exit(1);
	      }
		   in->MapTo(map);
	      in->WriteXWD();
			continue;
		}
		if (strcasecmp(argv[i], "-out") == 0) {
			if (++i >= argc) usage("-out must be followed by new name");
			outName = argv[i]; 
	    	//  write it out 
	    	if (outName) {
	       	in->name = outName;
	       	in->WriteXWD();
	    	}
			continue;
		}
		if (strcasecmp(argv[i], "-over15") == 0) { 
		   over15 = True; 	
	      int count = 0, siz = in->image->width * in->image->height;
	      for (i = 0; i < siz; i++) 
			if ((unsigned char)in->image->data[i] > 15) count++;
	      printf("%d\n", count);
			continue;
		}
	 	if (strcasecmp(argv[i], "-name") == 0) {
	   	if (++i >= argc) usage("");
	   	newName = argv[i];
	   	//  set the name
	   	in->name = newName;
			continue;
	   }
	 	if (strcasecmp(argv[i], "-ncolors") == 0) {
	    	printf ("%d\n", in->ncolors); 
	    	if (ncolors > -1) {
				XColor *oldColors = in->colors;
	        	if (ncolors > 0) {
		  		XColor *newColors = new XColor [ncolors];
	         memset (newColors, 0, in->ncolors * sizeof(XColor));
	         memcpy (newColors, in->colors, in->ncolors * sizeof(XColor));
	         in->colors = newColors;
	      } else in->colors = NULL;		// No Colors!!!
				delete oldColors;
	         in->ncolors = in->header->ncolors = ncolors;
	    	}
			continue;
	   }
	   if (strcasecmp(argv[i], "-path") == 0) {
	   	if (++i >= argc) usage("");
	   	newPath = argv[i];     
	
			//  set the path
	    	in->path = newPath;
			continue;
	   }
		if (strcasecmp(argv[i], "-patch") == 0) {
		    if (++i >= argc) usage("");
		    patchName = argv[i]; 		
			continue;
		}
		if (strcasecmp(argv[i], "-patchCentered") == 0) {
		    if (++i >= argc) usage("");
		    patchCenteredName = argv[i]; 	
			continue;
		}
		if (strcasecmp(argv[i], "-pt") == 0) {
		    if (++i >= argc) usage("");
		    pt = argv[i]; 
	      char *newfont = new char [256];
			sprintf (newfont, "%s%s%s.bdf", family, style, pt);
	    	in->SetFont(newfont);
			continue;
		}
		if (strcasecmp(argv[i], "-rect") == 0) {
		   rect = True; 
		   if (i+4 >= argc) usage("-rect requires followed by \"x1 y1 x2 y2\" ");
		   x1 = atoi(argv[++i]); y1 = atoi(argv[++i]); 	
			x2 = atoi(argv[++i]);
		   y2 = atoi(argv[++i]); 	
	   	in->DrawRect(x1, y1, x2, y2); 
			continue;
		}
		if (strcasecmp(argv[i], "-resize") == 0) {
		   if (i+2 >= argc) usage("");
		   resize = True;   
		   w = atoi(argv[++i]); 
		   h = atoi(argv[++i]); 		
	   	in->Resize (w,h); 
			continue;
		}
		if (strcasecmp(argv[i], "-sample") == 0) {
		   sample = True; 			
	   	//  resize it in halve by sampling and
	   	if (sample) 		in->Sample(); 
			continue;
		}
	   if (strcasecmp(argv[i], "-scale") == 0) {
	      if (++i >= argc) usage("");
	      scale = atof(argv[i]);              
	   	in->Scale (scale);
			continue;
	   }
		if (strcasecmp(argv[i], "-put") == 0) {
		   if (i+4 >= argc) usage("");
		   put = True;   
			cell = atoi(argv[++i]);
		   newred = atoi(argv[++i]); newgreen = atoi(argv[++i]);
		   newblue = atoi(argv[++i]); 		
			in->colors[(Pixel)cell].red = (Color) newred,
			in->colors[(Pixel)cell].green = (Color) newgreen, 
			in->colors[(Pixel)cell].blue = (Color) newblue;
		    continue;
		}
		if (strcasecmp(argv[i], "-set") == 0) {
		   set = True;  
	    	in->Set();
		   continue;
		}
		if (strcasecmp(argv[i], "-shrink") == 0) {
		   shrink = True; 	
	    	//in->Shrink(); 
			continue;
		}
		if (strcasecmp(argv[i], "-swapbw") == 0) {
			swapBW = True; 
	   	if (swapBW) { in->SwapBW(); }
			continue;
		}
		if (strcasecmp(argv[i], "-showfonts") == 0) {
		   lsfonts = True; 
			continue;
		}
		if (strcasecmp(argv[i], "-squish") == 0) {
		   squish = True; 
	    	in->SquishCmap(); 
			continue;
		}
		if (strcasecmp(argv[i], "-style") == 0) {
		   if (++i >= argc) usage("");
		   style = argv[i]; 
	      char *newfont = new char [256];
			sprintf (newfont, "%s%s%s.bdf", family, style, pt);
	    	in->SetFont(newfont);
			continue;
		}
		if (strcasecmp(argv[i], "-text") == 0) {
		   if (++i >= argc) usage("");
		   text = argv[i]; 
	   	in->DrawString(text, x, y, alignment); 
			continue;
		}
		if (strcasecmp(argv[i], "-trunc") == 0) {
		   if (++i >= argc) usage("");
		   trunc = atoi(argv[i]); 
			// truncate the colormap to the given # of entries
	   	if (trunc >= 0) in->ncolors=trunc;
			continue;
		}
		if (strcasecmp(argv[i], "-trim") == 0) {
		   trim = True; 	
	    	in->Trim(); 
			continue;
		}
		if (strcasecmp(argv[i], "-use4") == 0) {
		   use4 = True; 	
	    	in->Use4();
			continue;
		}
		if (strcasecmp(argv[i], "-uniq") == 0) {
		    uniq = True; 
			continue;
		}
		if (strcasecmp(argv[i], "-use8") == 0) {
		    use4 = True; 
	    	in->Use8();
			continue;
		}
		if (strcasecmp(argv[i], "-verbose") == 0) {
		    verbose = True; 
			in->verbose = 1;
			continue;
		}
	   if (strcasecmp(argv[i], "-write") == 0) {
			write = True;  
	    	//  write it out to new name
	    	in->WriteXWD();
			continue;
	   }
		if (strcasecmp(argv[i], "-x") == 0) {
		    if (++i >= argc) usage("");
		    x = atoi(argv[i]); 
		}
		if (strcasecmp(argv[i], "-y") == 0) {
		   if (++i >= argc) usage("");
		   y = atoi(argv[i]); 			
			continue;
		}
		if (strcasecmp(argv[i], "-x2") == 0) {
		   if (++i >= argc) usage("");
		   x2 = atoi(argv[i]); 		
			continue;
		}
		if (strcasecmp(argv[i], "-y2") == 0) {
		   if (++i >= argc) usage("");
		   y2 = atoi(argv[i]); 		
			continue;
		}
		if (strcasecmp(argv[i], "-x1") == 0) {
		   if (++i >= argc) usage("");
		   x1 = atoi(argv[i]); 		
			continue;
		}
		if (strcasecmp(argv[i], "-y1") == 0) {
		   if (++i >= argc) usage("");
		   y1 = atoi(argv[i]); 		
			continue;
		}
		usage("");
	}
   if (!fileName) usage("");
    


    // TODO // now handle colormap changes 
    // TODO if (wp >= 0) in->whitePixel = (Pixel) wp;
    // TODO if (bp >= 0) in->blackPixel = (Pixel) bp;



}
