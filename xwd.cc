
/* xwd.cc - class to represent XWD images  */

static const char *dxwd_cc_RCSID="$Id: xwd.cc,v 1.13 2019/03/01 23:15:23 dkl Exp $";

#define  c_plusplus

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <dirent.h>

#include "zlib.h"
#include "xwd.h"
#include "font.h"
#include "ppm.h"
#include "rgb.h"

unsigned long swaptest = 1;

//  constructor
xwd::xwd (const char *in_name) { 
    image = NULL; colors = NULL; ncolors = 0; header = NULL;
    name = new char [strlen(in_name)+1];
    strcpy (name, in_name);
    font = new char [strlen(DEFAULT_FONT)+1];
    strcpy (font, DEFAULT_FONT);
    fontPath = new char [strlen(DEFAULT_FONT_PATH)+1];
    strcpy (fontPath, DEFAULT_FONT_PATH);
    fontDPI = new char[256]; 
    sprintf (fontDPI, "misc");
    ReadXWD(0);
}

//
//  destructor
//
xwd::~xwd () { 
    if (font) delete font;
    if (fontDPI) delete fontDPI;
    if (fontPath) delete fontPath;
    if (colors) delete colors;
    if (header) delete header;
    if (image) if (image->data) delete image->data;
    if (image) delete image;
    if (name) delete name;
}

//
//  SetName sets the name
//
void xwd::SetName (char *newname) {
    if (!newname) return;
    if (name) delete name;
    name = new char [strlen(newname)+1];
    strcpy (name, newname);
}

//  SetPath sets the path
void xwd::SetPath (char *newpath) {
    if (!newpath) return;
    if (path) delete path;
    path = new char [strlen(newpath)+1];
    strcpy (path, newpath);
}

//  SetFont sets the font
void xwd::SetFont (char *newfont) {
    if (!newfont) return;
    if (font) delete font;
    font= new char [strlen(newfont)+1];
    strcpy (font, newfont);
}

//  SetFont sets the font
void xwd::SetFontPath (char *newpath) {
    if (!newpath) return;
    if (fontPath) delete fontPath;
    fontPath = new char [strlen(newpath)+1];
    strcpy (fontPath, newpath);
}

//  SetFontDPI sets the fontDPI
void xwd::SetFontDPI (char *newdpi) {
    if (!newdpi) return;
    if (fontDPI) delete fontDPI;
    fontDPI = new char [strlen(newdpi)+1];
    strcpy (fontDPI, newdpi);
}


int xwd::ListPath (char *path) {

	//static int depth=0;
	static int depth=-1, indent=0;

	struct dirent *de;  // Pointer for directory entry 
	depth++;
	//indent = 2 * (depth-1);
	//indent = (depth) * 2;
	indent = depth*2;
  
	if (debug) printf("in ListPath, depth=%d, indent=%d\n", depth, indent); 

	// opendir() returns a pointer of DIR type.  
	DIR *dr = opendir(path); 
  
	// opendir returns NULL if couldn't open directory 
	if (dr == NULL)  { 
		fprintf(stderr,"Error: Could not open font path %s\n", fontPath ); 
		depth--;
		indent=(depth)*2;
		return 1; 
	} 
  
	while ((de = readdir(dr)) != NULL)  {
		if (strcmp(de->d_name,".") && strcmp(de->d_name,"..")) {
			int newlen = strlen(de->d_name)+strlen(fontPath)+2;
			char *newname = new char [strlen(de->d_name)+strlen(fontPath)+2];
			sprintf (newname, "%s/%s", path,de->d_name);
		
			// is it a directory or a file?  Try opening it.
			if ( DIR *dr2 = opendir(newname))  {
				// yes it did.  close it now that it's open, and recurse into it.
				closedir(dr2);
				// tired of wrestling with printf 
				if (verbose) for (int i=0;i<indent;i++) { printf (" "); }
				if (verbose) printf("%s:\n", de->d_name); 
				if (debug) if (verbose) 
					printf("%d %d %*s:\n", depth, indent, indent, de->d_name); 
				ListPath(newname);
			} else {
				if (strstr(de->d_name,".bdf")) {
					if (verbose) for (int i=0;i<indent;i++) { printf (" "); }
					if (verbose) {
						printf("%s", de->d_name); 
					} else{
						char *shortname = new char [strlen(de->d_name)+1];
						strcpy(shortname,de->d_name);
						shortname [strlen(de->d_name)-4] = '\0';
						printf("%s", shortname); 
						delete [] shortname;
					}
					printf("\n" ); 
				}
				if (debug) if (verbose)
					printf("%d %d %*s\n", depth, indent, indent, de->d_name); 
			}
			delete [] newname;
		}
	}
	closedir(dr);
	depth--;
	indent=(depth)*2;
	return 0;
}

//  ShowFonts lists the fonts found at $fontpath
int xwd::ShowFonts () {
    if (fontPath)
    	ListPath(fontPath);
	else
    	ListPath((char *)"");
    return 0;
}

//  GetClosestColor takes in rgb values from 0 - 255
int xwd::GetClosestColor (int r, int g, int b) {
    int	i, j, closest=0;
    double best=0.0;
   
    // X uses 16 bits for color 
    r *= 255; g *= 255; b *= 255;

    for (i=0; i<ncolors; i++)   		
	if ((colors[i].red==r) && (colors[i].green==g) && (colors[i].blue==b)) 
            return i;

    for (i=0; i<ncolors; i++) {
        double redDistance = (double)(abs(colors[i].red - r));
        double greenDistance = (double)(abs(colors[i].green - g));
        double blueDistance = (double)(abs(colors[i].blue - b));
        double redGreenDistance =
            sqrt((redDistance * redDistance) * (greenDistance * greenDistance));        double distance =
            sqrt ((blueDistance * blueDistance) +
                  (redGreenDistance * redGreenDistance));

        if (i == 0) { best = distance; closest = i; }
        else if (distance < best) { best = distance; closest = i; }
    }
    return closest;
}

//  Dump the contents of the cmap
void xwd::DumpCmap (int cells) {
    int	i, j, closest, distance;

    printf ("Number of colors: %d   whitePixel: %d    blackPixel: %d\n", 
		ncolors, (int)whitePixel, (int)blackPixel);
    printf ("\t16b\t16b\t16b\tR-MSB\tR-LSB\tG-MSB\tG-LSB\tB-MSB\tB-LSB\n" );

    for (i=0; i< min(cells,ncolors); i++)   		
      printf ("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", i,
		//colors[i], colors[i+16], colors[i+32], colors[i+48],
		colors[i].red, colors[i].green, colors[i].blue,
		colors[i].red/256, colors[i].red%256, colors[i].green/256, colors[i].green%256,
		colors[i].blue / 256, colors[i].blue % 256);
	
}

//  takes in rgb values from 0 - 255
int xwd::GetFarthestColor (int r, int g, int b) {
    int	i, j, farthest, distance;

    // X uses 16 bits for color 
    r *= 255; g *= 255; b *= 255;
    farthest = 0; distance = 0;
    for (i=0; i<ncolors; i++) {
	int d = abs(colors[i].red - r) + abs(colors[i].green - g) +
				abs(colors[i].blue - b);
    	if (d > distance) { distance = d; farthest = i; }
    }
    return farthest;
}

//  mapTo remaps a xwd's cmap to that of toMe as necessary
void xwd::MapTo (xwd *toMe) {
    unsigned char        *map;
    XColor		*newColors;
    double		*distances;
    int			i, j, closest, needed=0;
    int			*mappings;

    if (debug)
        printf ("%s: cmap has %d cells, target has %d cells\n", name, ncolors, toMe->ncolors);
    distances = new double [toMe->ncolors];
    mappings = new int [ncolors];

    // does anything need to be done at all?
    if (toMe->ncolors < ncolors) needed = 1;  	// it does if one is smaller 
    else for (i=0; i<ncolors; i++)   		// or if they don't match %100 
	if ((colors[i].red != toMe->colors[i].red) ||
            (colors[i].green != toMe->colors[i].green) ||
            (colors[i].blue != toMe->colors[i].blue)) 
            needed=1;

    if (needed) {

    // for each color in the old cmap, find the closest match in the new 
    for (i=0; i<ncolors; i++) {
        closest = 0; 
        for (j=0; j<min(toMe->ncolors,16); j++) {
        //for (j=0; j<toMe->ncolors; j++) {
	    distances[j] = abs(colors[i].red - toMe->colors[j].red) +
	    		abs(colors[i].green - toMe->colors[j].green) +
			abs(colors[i].blue - toMe->colors[j].blue);

/*
	    distances[j] = sqrt
	    ((abs(colors[i].red - toMe->colors[j].red) * abs(colors[i].red - toMe->colors[j].red)) +
	    (abs(colors[i].green - toMe->colors[j].green) * abs(colors[i].green - toMe->colors[j].green)) +
	    (abs(colors[i].blue - toMe->colors[j].blue) * abs(colors[i].blue - toMe->colors[j].blue)) );
*/
            if (distances[j] < distances[closest]) closest = j;
        }
        mappings[i] = closest;
        if (verbose)
            fprintf (stderr,"%s: mapping cell %d to %d\n", name, i, closest);
    }

    // remap each pixel
    for (int x=0; x < image->width; x++) {
      for (int y=0; y < image->height; y++) {
        Pixel pix;
        pix = GetPixel(x, y);
 	if (pix != (Pixel)mappings[(unsigned char)pix]) {
          PutPixel (x, y, (Pixel)mappings[(unsigned char)pix]);
        }
      }
    }

    newColors = new XColor [toMe->ncolors];
    memcpy (newColors, toMe->colors, toMe->ncolors * sizeof(XColor));
    delete colors;
    colors = newColors;
    header->ncolors = ncolors = toMe->ncolors;

    // now must recalibrate whitePixel and blackPixel
    whitePixel = toMe->whitePixel; blackPixel = toMe->blackPixel;
/*
    whitePixel = 0; blackPixel = 1;
    for (i = ncolors-1; i >= 0; i--) {
	if ((colors[i].red==0) && (colors[i].green==0) && (colors[i].blue==0))
	    blackPixel = i;
	//if ((colors[i].red/256==256) && (colors[i].green/256==256) && (colors[i].blue/256==256)) 
	    //whitePixel = i;
	if ((colors[i].red==65535) && (colors[i].green==65535) && (colors[i].blue==65535)) 
	    whitePixel = i;
	if ((colors[i].red==65280) && (colors[i].green==65280) && (colors[i].blue==65280)) 
	    whitePixel = i;
	if (debug)fprintf(stderr,"ReadXWD: whitePixel=%d, blackPixel=%d\n",
		whitePixel,blackPixel);
	}
*/
    }
    fg = blackPixel; 	bg = whitePixel;
    delete mappings;
    delete distances;
}

//  HasColor returns 1 if the color exists in the colormap or else 0
int xwd::HasColor (XColor *hasMe) {
    XColor	*newcolors;
	int i;

    for (i=0; i<ncolors; i++)          
    	if ((colors[i].red == hasMe->red) &&
            (colors[i].green == hasMe->green) &&
            (colors[i].blue == hasMe->blue))
            return 1;

	return 0;
}

//  GetColor returns the pixel value of the desired color
//  or ncolors if it does not exist in the colormap
Pixel xwd::GetColor (int r, int g, int b) {
    int 	n=0, found=0;

    while ((n<ncolors) && (!found)) {
        found = ((colors[n].red==r) && 
		 (colors[n].green==g) && 
		 (colors[n].blue==b));
        n++;
    }
	// return Pixel value or -1 if not found
   return found ? n : -1;
}

//  GetColor returns the pixel value of the desired color
//  or ncolors if it does not exist in the colormap
Pixel xwd::GetColor (char *colorname) {
    int 	n=0, found=0;
    int		r,g,b;

    //GetRGB (colorname, &r, &g, &b);

    while ((n<ncolors) && (!found)) {
        found = ((colors[n].red==r*256+r) && 
		 (colors[n].green==g*256+g) && 
		 (colors[n].blue==(b<<8)+3));
        if (!found) n++;
    }
    return found ? n : -1;
}

//  ReadRGB() reads r, g, and b values from rgb.txt 
//  returns 0 for success, -1 for error
int xwd::ReadRGB (char *ofMe, int *r, int *g, int *b) {
    char 	buf[512], color[1024], c1[1024], c2[1024];
    FILE 	*rgb;
    int 	w;

	if ((rgb = fopen("rgb.txt", "rb"))) {
		while ((w=fscanf (rgb, "%[^\n]\n", buf))==1) {
			if (sscanf(buf, "%d %d %d\t\t%s %s\n", r, g, b, c1, c2)==5)
				continue;
			if (sscanf(buf, "%d %d %d\t\t%s\n", r, g, b, color)==4) {
				if (strcmp(ofMe, color)==0) {
					fclose(rgb);
					return 0;
				}
			}
		}
		fclose(rgb);
	}
	return -1;
}


//  numRGB returns the enumber of RGB specifications in our table
int xwd::NumRGBs () {
	if (debug) 
		printf ("number of rgbs = %d / %d \n", sizeof(rgbs) , sizeof(rgbSpec));
 	return sizeof(rgbs) / sizeof(rgbSpec);
}


//  FindRGBByName finds an RGB spec in the table
//  and returns 0 for success, -1 for error
int xwd::FindRGBByName (char *ofMe) {
   FILE 	*rgb;  
	int i=0,numrgbs;
	Bool found=False;

	numrgbs = NumRGBs();
	i=0; while ((i<numrgbs) && strcasecmp(ofMe, rgbs[i].name)) {
		i++;
	}   
	return (i<numrgbs) ? i : -1;
}

//  lsColors 
int xwd::lsColors () {
	int i=0;
	for (i=0; i<NumRGBs(); i++) {
		printf ("%3d %3d %3d\t%s\n", rgbs[i].r,rgbs[i].g,rgbs[i].b,rgbs[i].name);
	}   
   return NumRGBs();
}


//  AddColor attempts to add the named color to the colormap 
//  and returns the number of colors added
int xwd::AddColor (char *addMe) {
    int 		i=0;
	Bool 	found=False;
	int numrgbs = NumRGBs();

	while (strcasecmp(rgbs[i].name, addMe)!=0) {
   	i++;
	} 

	if (i<NumRGBs()) 
   	return AddColor(rgbs[i].r, rgbs[i].g, rgbs[i].b);
   else 
		return 0;
}

//  addColor adds a color to the colormap & returns the number of colors added
int xwd::AddColor (int r, int g, int b) {
    XColor	nc;

    if (ncolors == 255) return 0;
    nc.pixel=(Pixel)ncolors;
    nc.red=(r*256)+r;
    nc.green=(g*256)+g;
    nc.blue=(b*256)+b;
    nc.flags=DoRed|DoGreen|DoBlue;

    return (AddColor(&nc));
}

//  addColor adds a color to the colormap & returns the number of colors added
int xwd::AddColor (XColor *addMe) {
    XColor	*newcolors;

    if (ncolors == 255) return 0;

    newcolors = new XColor [ncolors+1];
    memcpy (newcolors, colors, ncolors * sizeof(XColor));
    delete colors;
    colors = newcolors;
    memcpy (newcolors+ncolors, addMe, sizeof(XColor));
    ncolors++;
    return 1;
}

//  MergeCmap merges the cmap into that of intoMe
void xwd::MergeCmap (xwd *intoMe) {
    int i;

    for (i=0; i<ncolors; i++)          
      if (! intoMe->HasColor(colors+i)) {
        intoMe->AddColor(colors+i);
	if (debug) 
	    printf ("MergeCmap: merging color %d from %s\n",i,intoMe->name);
	}
}

//  uniq returns the number of uniq colors
int xwd::Uniq () {
    int 	i, j, numdups=0, unique=0;
    unsigned char	*map;

    map = new unsigned char [ncolors];
    for (i=0; i<ncolors; i++) map[i]=0;
    for (i=0; i<ncolors; i++) {
      for (j=i+1; j<ncolors; j++) {
        if (map[j] != 99) 
        if ((colors[i].red == colors[j].red) &&
            (colors[i].green == colors[j].green) &&
            (colors[i].blue == colors[j].blue)) {
          numdups++;
	  map[j]=99;	// so we don't count it twice
	}
      }
    }
    unique = ncolors - numdups;
    return unique;
}

//  SetWhitePixel sets the white pixel to the given value
//  returns 0 for success and -1 for error
int xwd::SetWhitePixel (Pixel pix) {
    if (pix >= ncolors) return -1;
    whitePixel = pix;
    colors[whitePixel].red=colors[whitePixel].green=colors[whitePixel].blue=65535;
    return 0;
}

//  blackPixel sets the blackPixel to the given value
int xwd::SetBlackPixel (Pixel pix) {
    if (pix >= ncolors) return -1;
    blackPixel = pix;
    colors[blackPixel].red=colors[blackPixel].green=colors[blackPixel].blue=0;
    return 0;
}

//  SwapBW swaps BlackPixel and whitePixel and remaps the pixels
int xwd::SwapBW () {
	register int x, y;

   // save the current values as SetWhitePixel and SetBlackPixel will change em
	Pixel currentWhitePixel = whitePixel;
	Pixel currentBlackPixel = blackPixel;

	SetBlackPixel(currentWhitePixel);
	SetWhitePixel(currentBlackPixel);

   // set all the white pixels to 99, which we don't use
	for (y=0; y < image->height ; y++) {
		for (x = 0; x < image->width; x++) {
			if (GetPixel(x,y) == (Pixel) currentWhitePixel)  PutPixel(x,y,99);
		}
	}

   // set all the black pixels to the new whitePixel
	for (y=0; y < image->height ; y++) {
		for (x = 0; x < image->width; x++) {
			if (GetPixel(x,y) == (Pixel) currentBlackPixel)  PutPixel(x,y,blackPixel);
		}
	}

   // Now set all the formerly white pixels to the new whitePixel
	for (y=0; y < image->height ; y++) {
		for (x = 0; x < image->width; x++) {
			if (GetPixel(x,y) == (Pixel) 99)  PutPixel(x,y,whitePixel);
		}
	}
	return 1;    // TODO:  use or discard this
}

//  SetForeground sets the foreround to the given pixel value
int xwd::SetForeground (Pixel pix) {
    if (pix >= ncolors) return -1;
    fg = pix; 		return 0;
}

//  SetForeground sets the foreround to the named color,
//  adding that color to the colormap only if necessary
int xwd::SetForeground (char *colorname) {
   Pixel p = GetColor(colorname);
   if (p < ncolors) { fg = p;  return 0; }
   if (AddColor(colorname) != -1) 
    	p = GetColor(colorname); 
   if (p < ncolors) { fg = p; return 0; }
	else return 1;
}

//  SetBackground sets the backround to the given pixel value
int xwd::SetBackground (Pixel pix) {
    if (pix >= ncolors) return -1;
    bg = pix; 		return 0;
}

//  squishCmap squishes out duplicate cells from the cmap
int xwd::SquishCmap () {
    int 	i, j, k, numdups, unique_colors;
    Bool	mapped;
    XColor	*newcolors;
    unsigned char        *map;

    if (debug) {
        printf ("squishCmap:  before: \n");
        for (i=0; i<ncolors; i++) 
          printf ("\tcell %d = %d, %d, %d, %d\n", i, colors[i].pixel,
		colors[i].red, colors[i].green, colors[i].blue);
    } 

    /* create a mapping of cells to duplicate cells  for image 1
        and mark unused cells by setting values to 99 */
    map = new unsigned char [ncolors];
    for (i=0; i < ncolors; i++) {
        map[i] = (unsigned char) i;    /* initialize to straight mapping */
        for (mapped=False,j=0; (mapped==False) && (j<i); j++) {
            if ((colors[i].red == colors[j].red) &&
                (colors[i].green == colors[j].green) &&
                (colors[i].blue == colors[j].blue)) {
                map[i] = (unsigned char) j;  mapped=True;
                if (debug) printf("cell %d in mapped to cell %d\n",i,j);
                colors[i].red = 999;
                colors[i].green = 999;
                colors[i].blue = 999;
            }
        }
    }
    //  count duplicate cells 
    for (numdups=0, i=1; i<ncolors; i++)
      if (colors[i].red == 999 && colors[i].red == 999 && colors[i].red == 999)
  	numdups++;
    unique_colors = ncolors - numdups;
    newcolors = new XColor [unique_colors];


    //  copy contents of cells which are not duplicates into new map
    for (i=0,k=0; i<ncolors; i++) {
        if ((colors[i].red != 999) ||
            (colors[i].green != 999) ||
            (colors[i].blue != 999)) {
	      newcolors[k].red = colors[i].red;
	      newcolors[k].green = colors[i].green;
	      newcolors[k].blue = colors[i].blue;
	      newcolors[k].flags = colors[i].flags;
	      newcolors[k].pixel = k;
	      map[i] = (unsigned char) k;
	      k++;
        } else   			// it was previously mapped above
	    map[i] = map[map[i]];
    }
    delete colors;
    colors = newcolors;
    header->ncolors = ncolors = unique_colors;

    if (debug) {
        printf ("squishCmap:  after: \n");
        for (i=0; i<ncolors; i++) 
          printf ("\tcell %d = %d, %d, %d, %d, %d\n",i,colors[i].pixel, map[i],
			 colors[i].red, colors[i].green, colors[i].blue);
    }

    if (numdups) {
    for (int x=0; x < image->width; x++) {
      for (int y=0; y < image->height; y++) {
        unsigned long pix;
        pix = GetPixel(x, y);
	PutPixel (x, y, (Pixel) map[pix]);
	if (debug)
	    if (debug) printf("resetting pix value from %d to %d\n", pix, map[pix]);
      }
    }
    }
   return 0;
}

int xwd::DrawLines (char *linesName) {
    typedef struct lineStruct {
        int x0, y0, x1, y1;
        struct lineStruct *next;
    } lineType;
    int numLines = 0, lineNo, l;
    char s[256];
    lineType *theLines;
    FILE *lines;
    
    /* open file to get number of lines */
    if (!(lines = fopen(linesName, "rb"))) {
        fprintf (stderr, "Error opening lines file\n"); return(1);
    }
    while (fgets (s, 256, lines)) numLines++;
    fclose(lines);

    /* malloc the storage for the endpoints */
    if ((theLines = new lineType [numLines]) == NULL) {
        fprintf (stderr, "Can't malloc lines buffer."); return(1); 
    }
    if (!(lines = fopen(linesName, "rb"))) {
        fprintf (stderr, "Error opening lines file\n"); return(1);
    }
    /*  read in $numLines lines  */
    lineNo = 0; 
    /*while (lineNo < numLines) {
        fgets (s, 256, lines);
        if (sscanf (s, "Line #%d from ( %d, %d) to (%d, %d)\n", &l, 
	        &theLines[lineNo].x0, &theLines[lineNo].y0,
	  	&theLines[lineNo].x1, &theLines[lineNo].y1) == 5 ) {
	    lineNo++;
        }
    } */
    while (sscanf (s, "Line #%d from ( %d, %d) to (%d, %d)\n", &l, 
	        &theLines[lineNo].x0, &theLines[lineNo].y0,
	  	&theLines[lineNo].x1, &theLines[lineNo].y1) == 5 ) {
	    lineNo++;
    } 
    fclose(lines);

    /*  draw the lines  */
    for (lineNo = 0; lineNo < numLines; lineNo++) {
	fprintf (stderr, "Line %d: %d,%d to %d,%d\n", lineNo,
			theLines[lineNo].x0, theLines[lineNo].y0,
			theLines[lineNo].x1, theLines[lineNo].y1);
	DrawLine (theLines[lineNo].x0, theLines[lineNo].y0,
		theLines[lineNo].x1, theLines[lineNo].y1);
    }
   return 0;
}

//
//  Clear fills the entire image with black
//
void xwd::Clear () {
    register int x, y;

    for (y=0; y < image->height; y++) {
        for (x = 0; x < image->width; x++) {
            image->data[y*image->width+x] = blackPixel;
        }
    }
}

//
//  Set fills the entire image with white
//
void xwd::Set () {
    register int x, y;

    for (y=0; y < image->height; y++) {
        for (x = 0; x < image->width; x++) {
            image->data[y*image->width+x] = whitePixel;
        }
    }
}

//  Fill fills the specified rect with the foreground color
int xwd::Fill (int x1, int y1, int x2, int y2) {
    register int x, y;

    //  first, put the corners in the correct order
    //  x1, x2 is the northwest corner, x2, y2 is the southeast
    if (x1 > x2) { int swap = x1; x1 = x2; x2 = swap; }
    if (y1 > y2) { int swap = y1; y1 = y2; y2 = swap; }

    //  check bounds
    x1 = max(x1,0); 	x2 = min(x2,image->width);
    y1 = max(y1,0); 	y2 = min(y2,image->height);

    for (y=y1; y <= y2 ; y++) {
        for (x = x1; x <= x2; x++) {
            image->data[y*image->width+x] = fg;
        }
    }
	return 1;    // TODO:  use or discard this
}

//  Crop clips the image to the specified rect
int xwd::Crop (int x1, int y1, int x2, int y2) {
   unsigned long pix;
   int newWidth, newHeight;
   register int x, y;
   char *newdata;

   //  first, put the corners in the correct order
   //  x1, x2 is the northwest corner, x2, y2 is the southeast
   //if (x1 > x2) { int swap = x1; x1 = x2; x2 = swap; }
   //if (y1 > y2) { int swap = y1; y1 = y2; y2 = swap; }


   // if x1 == image->width, this comes from TrimLeft(); nothing to trim
   if (x1 > image->width-1) return 1;
   // if x2 == -1, this probably comes from TrimRight(); nothing to trim
   if (x2 < 0) return 1;
   // if y1 == image->height, this comes from TrimTop(); nothing to trim
   if (y1 > image->height-1) return 1;
   // if y2 == -1, this comes from TrimBottom(); nothing to trim
   if (y2 < 0) return 1;

   // these out-of-bounds cases should probably also be ignored
   if (x1<0) return 1;
   if (x2>image->width-1) return 1;
   if (y1<0) return 1;
   if (y2>image->height-1) return 1;

   // set up distances.  Add one to operate inclusively
   newWidth = (x2 - x1) + 1; 	newHeight = (y2 - y1) + 1;

   //newimg = new XImage;
   newdata = new char [newWidth * newHeight];
   memset (newdata, 0, newWidth * newHeight);

   for (y=0; y < newHeight ; y++) {
      for (x = 0; x < newWidth; x++) {
         pix = GetPixel (x + x1, y + y1);
         newdata[(y*newWidth)+x] = pix;
      }
   }
   image->width = newWidth;
   image->height = newHeight;
   image->bytes_per_line = newWidth;
   header->pixmap_width = (CARD32) newWidth;
   header->pixmap_height = (CARD32) newHeight;
   header->window_width = (CARD32) newWidth;
   header->window_height = (CARD32) newHeight;
   header->bytes_per_line = (CARD32) newWidth;

   delete image->data;
   image->data = newdata;
   return 0;
}

//
//  Halve halves the width and height of the image 
//  
int xwd::Halve () {
   char 	*newdata;
   register int x, y;

   int half_w = (image->width +1) / 2;
   int half_h = (image->height +1) / 2;

   for (y=0; y < image->height; y+=2) {
      for (x = 0; x < image->width; x+=2) {
         int cur = (y * image->width) + x;		// current position

         //  we are halving the image, so we could do it by sampling,
         // and just taking one out of every four pixels.  But it is grainy.
         //  Look for the darkes of the four instewad and use that.
  
	      Pixel pel[4]; 
         //  the four pixels are this one, the one to thr right,
         //  the one below and the one below and to the right.
         pel[0] = GetPixel (x,y);
         pel[1] = GetPixel (x+1,y);
         pel[3] = GetPixel (x, y+1);
         pel[4] = GetPixel (x+1, y+1);

         //  We now have four pixel values.  Find the closest to black.
         //  Black is (0, 0, 0) so the smallest is closest.
         for (int i = 1; i < 4; i++) {
            unsigned short r, g, b;
            r=colors[pel[0]].red;g=colors[pel[0]].green;b=colors[pel[0]].blue;
            int dist = (int) sqrt((r*r) + (b*b) + (g*g));

            r=colors[pel[i]].red, g=colors[pel[i]].green; b=colors[pel[i]].blue;
            int dist2 = (int) sqrt((r*r) + (b*b) + (g*g));

            if ( dist2 < dist ) {
               pel[0] = pel[i];
            }
         }
         image->data[cur]=(char)pel[0];
      }
   }
   image->width = half_w;
   image->height = half_h;
   image->bytes_per_line = half_w;
   header->pixmap_width = (CARD32) half_w;
   header->pixmap_height = (CARD32) half_h;
   header->window_width = (CARD32) half_w;
   header->window_height = (CARD32) half_h;
   header->bytes_per_line = (CARD32) half_w;

   delete image->data;
   image->data = newdata;
   return 0;
}

//
//  Sample halves the width and height of the image 
//  
int xwd::Sample () {
   register int x, y;
    char 	*newdata, *newPtr;

   int half_w = (image->width +1) / 2;
   int half_h = (image->height +1) / 2;

   newPtr = newdata = new char[half_w * half_h];
   memset (newdata, 0, half_w * half_h);

   int counter=0;
   //for (y=0; y < image->height; y+=2) {
   //   for (x = 0; x < image->width; x+=2) {
   //      int cur = (y * image->width) + x;		// current position
   //      //*newPtr++ = image->data[cur];
   //      newdata [counter++] = image->data[cur];
   //   }
  // }

   for (y=0; y < half_h; y++) {
      for (x = 0; x < half_w; x++) {
         int cur = (y*2 * image->width) + x*2;		// current position
         newPtr [y * half_w + x] = image->data[cur];
         //newdata [counter++] = image->data[cur];
      }
   }

   image->width = half_w;
   image->height = half_h;
   image->bytes_per_line = half_w;
   header->pixmap_width = (CARD32) half_w;
   header->pixmap_height = (CARD32) half_h;
   header->window_width = (CARD32) half_w;
   header->window_height = (CARD32) half_h;
   header->bytes_per_line = (CARD32) half_w;

   return 0;
}

//  Resize resizes the image to the specified rect
//  and backfills from the original image as possible
int xwd::Resize (int newWidth, int newHeight) {
    char 	*newdata;
    register int x, y;

    newdata = new char[newWidth * newHeight];
    memset (newdata, 0, newWidth*newHeight);

    for (y=0; y < newHeight ; y++) {
        for (x = 0; x < newWidth; x++) {
	  if ((x<image->width) && (y<image->height))
            newdata[y*newWidth+x] = image->data[y*image->width+x];
        }
    }
    image->width = newWidth;
    image->height = newHeight;
    image->bytes_per_line = newWidth;
    header->pixmap_width = (CARD32) newWidth;
    header->pixmap_height = (CARD32) newHeight;
    header->window_width = (CARD32) newWidth;
    header->window_height = (CARD32) newHeight;
    header->bytes_per_line = (CARD32) newWidth;

    delete image->data;
    image->data = newdata;
    return 0;
}

//  Scale scales the image to the specified rect
int xwd::Scale (double scale) {
    XImage *newimg;
    Pixel pix;
    register int x, y;

    // set up distances
    int newwidth = (int) ( (double)image->width * scale) ;
    int newheight = (int) ( (double)image->height * scale) ;

    for (y=0; y < newheight ; y++) {
        int oldy = (int)(((double)y)/scale);
        for (x = 0; x < newwidth; x++) {
            int oldx = (int)(((double)x)/scale);
            unsigned char c = image->data[(oldy*image->width)+oldx];
            image->data[(y*newwidth)+x] = c;
        }
    }
    image->width = newwidth; image->height = newheight;
    image->bytes_per_line = newwidth;
    header->pixmap_width = (CARD32) newwidth;
    header->pixmap_height = (CARD32) newheight;
    header->window_width = (CARD32) newwidth;
    header->window_height = (CARD32) newheight;
    header->bytes_per_line = (CARD32) image->bytes_per_line;
    header->bits_per_pixel = (CARD32) image->bits_per_pixel;
    header->window_width = (CARD32) newwidth;
    header->window_height = (CARD32) newheight;

    return 0;
}

//  Dump the image info
void xwd::DumpImage () {
    printf ("image: %x\n", image);
    printf ("image->width: %d\n", image->width);
    printf ("image->height: %d\n", image->height);
    printf ("image->xoffset %d\n", image->xoffset);
    printf ("image->format %d\n", image->format);
    printf ("image->data: %x\n", image->data);
    printf ("image->byte_order: %d\n", image->byte_order);
    printf ("image->bitmap_unit: %d\n", image->bitmap_unit);
    printf ("image->bitmap_bit_order: %d\n", image->bitmap_bit_order);
    printf ("image->bitmap_pad: %d\n", image->bitmap_pad);
    printf ("image->depth: %d\n", image->depth);
    printf ("image->bytes_per_line: %d\n", image->bytes_per_line);
    printf ("image->bits_per_pixel: %d\n", image->bits_per_pixel);
    printf ("image->red_mask: %lu\tgreen_mask: %lu\t blue_mask: %lu\n", image->red_mask, image->blue_mask, image->green_mask);
    printf ("image->obdata: %x\n", image->obdata);
    printf ("image->f: %x\n", image->f);
}

//  Dump the header
void xwd::DumpHeader () {
    printf ("header->header_size: %d\n", header->header_size);
    printf ("header->file_version: %d\n", header->file_version);
    printf ("header->pixmap_format: %d\n", header->pixmap_format);
    printf ("header->pixmap_depth: %d\n", header->pixmap_depth);
    printf ("header->pixmap_width: %d\n", header->pixmap_width);
    printf ("header->pixmap_height: %d\n", header->pixmap_height);
    printf ("header->xoffset %d\n", header->xoffset);
    printf ("header->byte_order: %d\n", header->byte_order);
    printf ("header->bitmap_bit_order: %d\n", header->bitmap_bit_order);
    printf ("header->bitmap_pad: %d\n", header->bitmap_pad);
    printf ("header->bits_per_pixel: %d\n", header->bits_per_pixel);
    printf ("header->bytes_per_line: %d\n", header->bytes_per_line);
    printf ("header->visual_class: %d\n", header->visual_class);
    printf ("header->red_mask: %d\tgreen_mask: %d\t blue_mask: %d\n", header->red_mask, header->blue_mask, header->green_mask);
    printf ("header->colormap_entries: %d\n", header->colormap_entries);
    printf ("header->ncolors: %d\n", header->ncolors);
    printf ("header->window_width: %d\n", header->window_width);
    printf ("header->window_height: %d\n", header->window_height);
}


//
//  Dump() 
//  Dump the pixel values
//
void xwd::Dump () {
	for (int x=0; x < image->width; x++) {
		for (int y=0; y < image->height; y++) {
			printf ("%d " , GetPixel(x, y));
		}
		printf ("\n");
	}
}

//  Dump the first line
void xwd::DumpLine (int line) {
	for (int x=0; x<image->width; x++) {
		printf ("%d ", GetPixel(x, line) );
   }
	printf ("\n");
}


//  alignment is LEFT RIGHT or CENTER
void xwd::DrawString(char *text, int x, int y, int alignment) {
	struct font *f = 0;

	#ifdef BUILTIN_FONT
		if (!font) font = copy_font (&builtin_font);
	#endif

	char *theFont = new char[256];
	sprintf (theFont, "%s/%s/%s", fontPath, fontDPI, font);
	f = read_bdf (theFont);
	draw_string (f, (unsigned char *)text, NULL, this,
						x, y, alignment, fg, bg, 255);
}

//
//  TrimLeft() trims an image on which a string has been drawn
//  by looking for solid color at the left edge to remove
//
void xwd::TrimLeft() {
	Bool allsame=True;
	int lineno = 0;

	while (allsame && (lineno < image->width)) {
		Pixel last = GetPixel(lineno,0);
   	for (int y=0; (y<image->height-1) && allsame; y++) {
			allsame = GetPixel(lineno,y) == last;
   	}
      // Crop() will crop inclcusively, so if we give it 0,0,width-1,hight-1,
      // the image should be unchanged.
      // This line below will leave the left value at image->width, which is
      // not actually on the image, if no column of pixels should be trimmed.  
		if (allsame) lineno++;
	}
 	if (verbose) 
      printf ("%s: can be trimmed down to %dx%d\n", 
                        name, image->width-(lineno+1),image->height);
   if ((lineno > 0) && (lineno < image->width))
   	Crop(lineno,0,image->width-1,image->height-1);
}

//
//  TrimRight() trims an image on which a string has been drawn
//  by looking for solid color at the right edge to remove
//
void xwd::TrimRight() {

	Bool allsame=True;
	int lineno = image->width-1;

	while (allsame && (lineno > -1)) {
		Pixel last = GetPixel(lineno,0);
   	for (int y=0; (y<image->height-1) && allsame; y++) {
			allsame = GetPixel(lineno,y) == last;
   	}
      // Crop() will crop inclcusively, so if we give it 0,0,width-1,hight-1,
      // the image should be unchanged.
      // This line below will leave lineno at -1 if no column shoud be trimmed
		if (allsame) lineno--;
	}
 	if (verbose) 
      printf ("%s: can be trimmed down to %dx%d\n",name,lineno+1,image->height);
   if (lineno < (image->width -1)) {
   	Crop(0,0,lineno,image->height-1);
	}
}

//
//  TrimTop() trims an image on which a string has been drawn
//  by looking for solid color at the right edge to remove
//
void xwd::TrimTop() {
	Bool allsame=True;
	int lineno = 0;

	while (allsame && (lineno < image->height)) {
		Pixel last = GetPixel(0,lineno);
   	for (int x=0; (x<image->width-1) && allsame; x++) {
			allsame = GetPixel(x,lineno) == last;
   	}
      // Crop() will crop inclcusively, so if we give it 0,0,width-1,hight-1,
      // the image should be unchanged.
      // this loop could leave lineno at image->height, in other words
      // pointing one past the bottom row, if no row should be trimmed
		if (allsame) lineno++;
	}
 	if (verbose) 
      printf ("%s: can be trimmed down to %dx%d\n", 
                                 name, image->width, image->height-lineno);
   if (lineno > 0)
   	Crop(0,lineno,image->width-1, image->height-1);
}

//
//  TrimBottom() trims an image on which a string has been drawn
//  by looking for solid color at the bottom edge to remove
//
void xwd::TrimBottom() {
	Bool allsame=True;
	int lineno = image->height-1;

	while (allsame && (lineno > 0)) {
		Pixel last = GetPixel(0,lineno);
   	for (int x=0; (x<image->width-1) && allsame; x++) {
			allsame = GetPixel(x, lineno) == last;
   	}
      // Crop() will crop inclcusively, so if we give it 0,0,width-1,hight-1,
      // the image should be unchanged.
      // this loop will leave lineno at -1 if no row should be trimmed
		if (allsame) lineno--;
	}
 	if (verbose) 
      printf ("%s: can be trimmed down to %dx%d\n", name, lineno);
   if (lineno < (image->height-1))
   	Crop(0,0,image->width-1,lineno);
}

//
//  Trim() trims an image on the right, left, top, and bottom
//  by looking for lines of solid color which can be removed
//
void xwd::Trim() {
   	TrimLeft();
   	TrimRight();
   	TrimTop();
   	TrimBottom();
}



//
//  PutPixel()
//  PutPixel sets the specified pixel to the specified value
//
void xwd::PutPixel (register int x, register int y, Pixel pix) {

   if (debug) fprintf (stderr, "PutPixel:  putting %lu at %d,%d\n", pix, x, y);

   if ((x<0) || (x>=image->width) || (y<0) || (y>=image->height)) return;

   if (image->bits_per_pixel == 8)
      image->data[(y*image->width)+x] = (unsigned char) pix;
   if (image->bits_per_pixel == 4) {
	   unsigned char p = image->data[((y*image->width)+x)/2];
      // is it the high 4 bits or the low 4 bits ?
      p &= (x/2*2 == x ? 240 : 15);
      // p now contains the four bits we want to preserve
      p += ((unsigned char) pix) >> (x/2*2 == x ? 4 : 0);

      image->data[((y*image->width)+x)/2] = (unsigned char) p;
   }
}


//
//  GetPixel()
//  GetPixel gets the value of the specified pixel 
//
//  Sets the specified pixel to the specified pixel value
Pixel xwd::GetPixel (register int x, register int y) {
    if ((x<0) || (x>=image->width) || (y<0) || (y>=image->height)) return 0;
    if (image->bits_per_pixel == 8)
	return (Pixel) image->data[(y*image->width)+x];
    if (image->bits_per_pixel == 4)
	return (x/2*2 == x) ? 
		(Pixel) ((image->data[((y*image->width)+x)/2] & 240) >> 4):
		(Pixel) (image->data[((y*image->width)+x)/2] & 15);
    // error, unknown depth
    return 0;
}

//  Converts an image to black and white
int xwd::MakeBW () {
    register int x, y;
    unsigned short threshold = 50000;

    for (y=0; y<image->height; y++) {
      for (x=0; x<image->width; x++) {
	unsigned char c = image->data[(y*image->width)+x];
 	//if (c != whitePixel) c = blackPixel;
 	if ((colors[c].red > threshold) ||
 	   (colors[c].green > threshold) ||
 	   (colors[c].blue > threshold)) {
	    c = whitePixel;
        } else c = blackPixel;
	image->data[(y*image->width)+x] = c;
      }
    }
    return 0;
}

//  Converts an image to four bits
int xwd::Use4 () {
    register int x, y;
    if (image->bits_per_pixel == 4)  return 0; 	// already is 4-bit

    char *newdata = new char[image->height*(image->width+1)/2];
    memset (newdata, 0, image->height*(image->width+1)/2);
    if (!newdata) return 1;
    for (y=0; y<image->height; y++) {
      for (x=0; x<image->width; x++) {
	unsigned char c = image->data[(y*image->width)+x];
	newdata[(y*int((image->width+1)/2))+(x/2)] += (c << ((((int)(x/2))*2 == x) ? 4 : 0));
      }
    }
    header->ncolors = ncolors = 16;
    header->bits_per_pixel = header->pixmap_depth = image->bits_per_pixel = 4;
    header->bytes_per_line = image->bytes_per_line = int((image->width+1)/2);
    header->pixmap_width = image->width; 
    header->pixmap_height = image->height; 
    header->byte_order = image->byte_order = MSBFirst;
    header->bitmap_pad = image->bitmap_pad = 8;
    header->bits_per_rgb = 4;
    delete image->data;
    image->data = newdata;
    return 0;
}

//  Converts an image to eight bits
int xwd::Use8 () {
    register int x, y;
    if (image->bits_per_pixel == 8)  return 0;  // already is 8-bit
                                                                                
    char *newdata = new char[(image->height*image->width)];
    memset (newdata, 0, (image->height*image->width));
    if (!newdata) return 1;
    for (y=0; y<image->height; y++) {
      for (x=0; x<image->width; x++) {
        unsigned char c = image->data[(y*image->width)+x];
        newdata[(y*image->height)+x] = ((x/2*2 == x) ? (c / 16) : (c % 16));
      }
    }
    header->ncolors = ncolors = 16;
    header->bits_per_pixel = header->pixmap_depth = image->bits_per_pixel = 8;
    header->bytes_per_line = image->bytes_per_line = image->width;
    delete image->data;
    image->data = newdata;
    return 0;
}

//  draws a rectangle
void xwd::DrawRect (int x0, int y0, int x1, int y1) {
   DrawLine (x0, y0, x0, y1); 
   DrawLine (x1, y0, x1, y1); 
   DrawLine (x0, y0, x1, y0); 
   DrawLine (x0, y1, x1, y1); 
}

//  draws a line using the appropriate routine
void xwd::DrawLine (int x0, int y0, int x1, int y1) {
    double dy, dx;  
    int x,y;
    
    //  start out with x0 <= x1 and y0 <= y1
    if (x0 > x1) { int swap = x0; x0=x1; x1=swap; }
    if (y0 > y1) { int swap = y0; y0=y1; y1=swap; }

    // dispense with the zero and infinite slope cases
    if (y0==y1) {for (x=x0; x<x1; x++) { PutPixel (x,y0,fg); } return;}
    if (x0==x1) {for (y=y0; y<y1; y++) { PutPixel (x0,y,fg); } return;}

    dy = (double) y1 - (double) y0;
    dx = (double) x1 - (double) x0;
    if (dy < dx) {
        DrawShallowLine (x0, y0, x1, y1);
    } else {
        DrawSteepLine (x0, y0, x1, y1);
    }
}

//  draws a line for the case where the slope is less than 1 
void xwd::DrawShallowLine (int x0, int y0, int x1, int y1) {
    int x, flipped = 0;
    double dy, dx, y, m;

    dy = (double) y1 - (double) y0;
    dx = (double) x1 - (double) x0;
    m = dy / dx;
    y = (double) y0;
    for (x = x0; x < x1; x++) {
        int yi = (int)(floor)(y + 0.5);
        PutPixel (x, yi, fg);
        y = y + m;
    }
}

//  draws a line for the case where the slope is greater than 1 
void xwd::DrawSteepLine (int x0, int y0, int x1, int y1) {
    int y, flipped = 0;
    double dy, dx, x, m;

    dy = (double) y1 - (double) y0;
    dx = (double) x1 - (double) x0;
    m = dx / dy;
    x = (double) x0;
    for (y = y0; y < y1; y++) {
        int xi = (int)(floor)(x + 0.5);
        PutPixel (xi, y, fg);
        x = x + m;
    }
}

// write an XWD image
int xwd::WriteXWD () {
    char *fname = new char [strlen(name)+64];
    FILE *out;
    gzFile gz_out;
    XWDColor xwdcolor;
    int i, buffer_size, result, ok;


    // force writing gzipped if name given ends in .gz 
    if (strstr(name, ".gz")) 	{ writeGzipped = True; }

    sprintf (fname, "%s", name); 
    if (writeGzipped) {
      if (!strstr(name, ".gz"))  strcat (fname, ".gz"); 
    }

    if (writeGzipped)  {
	gz_out = gzopen(fname, "wb");
        if (gz_out == NULL) {
          fprintf(stderr,"%s: Error: Can't write output file: %s\n",name,fname);
	  delete fname;
          return(1);
        }
    } else {
      if ((out = fopen (fname, "w" )) == NULL) {
        fprintf(stderr, "%s: Error: can't write output file: %s\n", name,fname);
        perror ("xwd");
	delete fname;
        return 1;
      }
    }

    // sync up first
    header->ncolors = ncolors;

    /*  Calculate header size  */
    header->header_size = (CARD32) SIZEOF(XWDheader) + strlen(name) + 1;

    if (*(char *) &swaptest) {
        _swaplong((char *) header, sizeof(*(header)));
        for (i = 0; i < ncolors; i++) {
            _swaplong((char *) &colors[i].pixel, sizeof(long));
            _swapshort((char *) &colors[i].red, 3 * sizeof(short));
        }
    }

    /*  Write the header */
    if (writeGzipped) {
	ok = (gzwrite(gz_out, (char *)header, SIZEOF(XWDheader)) == 
		SIZEOF(XWDheader));
	ok = ok && (gzwrite(gz_out, (char *)name, strlen(name)+1) == 
 			strlen(name)+1);
    } else {
        ok = (fwrite((char *)header, SIZEOF(XWDheader), 1, out) == 1);
        ok = ok && (fwrite(name, strlen(name) + 1, 1, out) == 1);
    }
    if (!ok) {
        perror("xwd");
        if (writeGzipped) { gzclose(gz_out); } else { fclose(out); }
	delete fname;
        return(1);
    }

    /*  Write the color map  */
    if (verbose) fprintf(stderr,"xwd: Dumping %d colors.\n", ncolors);
    for (i = 0; i < ncolors; i++) {
        xwdcolor.pixel = colors[i].pixel;
        xwdcolor.red = colors[i].red;
        xwdcolor.green = colors[i].green;
        xwdcolor.blue = colors[i].blue;
        xwdcolor.flags = colors[i].flags;
        if (writeGzipped) {
	    ok = (gzwrite(gz_out, (char *) &xwdcolor, SIZEOF(XWDColor)) == 
			SIZEOF(XWDColor));
        } else {
	    ok = (fwrite((char *) &xwdcolor, SIZEOF(XWDColor), 1, out) == 1);
	}
        if (!ok) {
            perror("xwd");
        if (writeGzipped) { gzclose(gz_out); } else { fclose(out); }
	    delete fname;
            return(1);
        }
    }
    /*  Write out the buffer.  */
    buffer_size = ImageSize();
    if (verbose) printf("xwd: Dumping pixmap.  bufsize=%d\n", buffer_size);

    if (writeGzipped) {
	ok = (gzwrite(gz_out, image->data, buffer_size) == buffer_size);
    } else {
	ok = (fwrite(image->data, (int) buffer_size, 1, out) == 1);
    }
    if (!ok) {
        perror("xwd");
        if (writeGzipped) { gzclose(gz_out); } else { fclose(out); }
	//delete fname;
        return (1);
    }
    if (writeGzipped) { gzclose(gz_out); } else { fclose(out); }
    //delete fname;
    return 0;
}

// taken from X11R6/programs/xwud.c 
int xwd::ReadXWD (int withData) {
    XWDColor		xwdcolor;
    int			i, buffer_size, win_name_size, result;
    FILE 		*in_file;
    gzFile 		gz_file;
    char 		*win_name, *gzname=NULL;
    Bool		use_gzipped = False;

    if (name == NULL)  return 1;
    if (gzname) delete gzname;
    if (strstr(name, ".gz")) {
        name[strlen(name)-3] = '\0';
    }
    if (!strstr(name, ".gz")) {
        if ((gzname = new char [strlen(name) + 8]) == NULL)
            Error("ReadXWD: can't malloc filename storage.");
        strcpy (gzname, name);  strcat (gzname, ".gz");
    }

    //  try to open a non-gzipped one first
    if (!(in_file = fopen(name, "rb"))) {
      if (!(gz_file = gzopen(gzname, "rb"))) {
	fprintf(stderr, "Error: Can't open %s or %s\n", name, gzname);
	delete gzname;
        return(1);
      }
      use_gzipped = True;	writeGzipped=True;
    }

    //  try to open a non-gzipped one first
    if (!(in_file = fopen(name, "rb"))) {
      if (!(gz_file = gzopen(gzname, "rb"))) {
	fprintf(stderr, "Error: Can't open %s or %s\n", name, gzname);
	delete gzname;
        return(1);
      }
      use_gzipped = True;	writeGzipped=True;
    }

    if (header) delete header;
    header = new XWDFileHeader;
    /* Read in header information.  */
    result = use_gzipped ? gzread(gz_file, (char *)header, SIZEOF(XWDheader)) :
                       Read((char *)header, SIZEOF(XWDheader), 1, in_file);

    if (!result) {
	fprintf(stderr, "Unable to read dump file header.\n");
	return 1;
    }

    if (*(char *) &swaptest)
        _swaplong((char *) header, SIZEOF(XWDheader));

    /* check to see if the dump file is in the proper format */
    if (header->file_version != XWD_FILE_VERSION) {
	fprintf(stderr, "Warning: XWD file format version mismatch.\n");
	//return 1;
    }
    if (header->header_size < SIZEOF(XWDheader)) {
	fprintf(stderr, "Error: XWD header size is too small.\n");
	return 1;
    }

    /* alloc window name */
    win_name_size = (header->header_size - SIZEOF(XWDheader));
    if ((win_name = new char [win_name_size]) == NULL)
        Error("Can't malloc window name storage.");

    /* read in window name */
    result = use_gzipped ? gzread(gz_file, win_name, win_name_size) :
       			Read(win_name, sizeof(char), win_name_size, in_file);
    if (!result && win_name_size)
        Error("Unable to read window name from dump file.");
    // name = win_name;

    /* initialize the input image */

    if (image) if(image->data) delete image->data;
    if (image) delete image;
    image = new XImage;
    image->depth = header->pixmap_depth;
    image->format = header->pixmap_format;
    image->xoffset = header->xoffset;
    image->data = NULL;
    image->width = header->pixmap_width;
    image->height = header->pixmap_height;
    image->bitmap_pad = header->bitmap_pad;
    image->bytes_per_line = header->bytes_per_line;
    image->byte_order = header->byte_order;
    image->bitmap_unit = header->bitmap_unit;
    image->bitmap_bit_order = header->bitmap_bit_order;
    image->bits_per_pixel = header->bits_per_pixel;
    image->red_mask = header->red_mask;
    image->green_mask = header->green_mask;
    image->blue_mask = header->blue_mask;

    /* read in the color map buffer */
    if(ncolors = header->ncolors) {
    // there should be at least white and black.  If there is less than
    // two colors, add white and black.
    if (ncolors<1) ncolors = 1;
        if (colors) delete colors;
	colors = new XColor [ncolors];
	if (!colors)
	    Error("Can't malloc color table");
	for (i = 0; i < ncolors; i++) {
	    result = use_gzipped ? 
		gzread(gz_file, (char *) &xwdcolor, SIZEOF(XWDColor)) :
		Read((char *) &xwdcolor, SIZEOF(XWDColor), 1, in_file);
	    if (!result) Error("Unable to read color map from dump file.");
	    colors[i].pixel = xwdcolor.pixel;
	    colors[i].red = xwdcolor.red;
	    colors[i].green = xwdcolor.green;
	    colors[i].blue = xwdcolor.blue;
	    colors[i].flags = xwdcolor.flags;
	    if (*(char *) &swaptest) {
	        //if (debug)
		    //fprintf(stderr,"ReadXWD: %s: swapping color values.\n", name);
		_swaplong((char *) &colors[i].pixel, sizeof(long));
		_swapshort((char *) &colors[i].red, 3 * sizeof(short));
	    }
	    if (debug) {
        	fprintf(stderr,"ReadXWD: color %d = %d,%d,%d\n",i,colors[i].red,colors[i].green,colors[i].blue);
            }
	}

	if (ncolors == 1) {
	    XColor *newcolors = new XColor[3];	
	    newcolors[0].red = colors[0].red;
	    newcolors[0].green = colors[0].green;
	    newcolors[0].blue = colors[0].blue;
	    newcolors[0].flags = colors[0].flags;
	    newcolors[0].pixel = 0;
	    newcolors[1].red = newcolors[1].green = newcolors[1].blue = 65535;
	    newcolors[1].pixel = 1;
	    newcolors[1].flags = colors[0].flags;
	    newcolors[2].red = newcolors[2].green = newcolors[2].blue = 0;
	    newcolors[2].pixel = 2;
	    newcolors[2].flags = colors[0].flags;
	    colors = newcolors;
	    header->ncolors = ncolors = 3;
 	}

	whitePixel = 0; blackPixel = 1;
	for (i = ncolors; i >= 0; i--) {
	    if ((colors[i].red==0) && (colors[i].green==0) && (colors[i].blue==0))
		blackPixel = i;
	    //if ((colors[i].red/256==256) && (colors[i].green/256==256) && (colors[i].blue/256==256)) 
		//whitePixel = i;
	    if ((colors[i].red==65535) && (colors[i].green==65535) && (colors[i].blue==65535)) 
		whitePixel = i;
	    if ((colors[i].red==65280) && (colors[i].green==65280) && (colors[i].blue==65280)) 
		whitePixel = i;
        if (debug)fprintf(stderr,"ReadXWD: whitePixel=%d, blackPixel=%d\n",
					(int)whitePixel, (int)blackPixel);
        }
    } else
	Error("No colors!");

    if (withData) {
      /* alloc the pixel buffer */
      buffer_size = ImageSize();
      if ((image->data = new char [buffer_size]) == NULL)
         Error ("Can't malloc data buffer.");

      /* read in the image data */
      if (use_gzipped) {
	result = gzread(gz_file, (char *)image->data, (int)buffer_size);
      } else { 
	result =Read((char *)image->data,sizeof(char),(int)buffer_size,in_file);
      }
      if (!result)
        Error("Unable to read pixmap from dump file.");
    }

    /* close the input file */
    use_gzipped ? (void) gzclose(gz_file) :
    (void) fclose(in_file);

    // squishCmap();
 
    return 0;
}

//   from X11R6/programs/xwud.c 
void xwd::Error (const char *str) {
    fprintf(stderr, "xwd: Error => %s\n", str);
    if (errno != 0) {
        perror("");
        fprintf(stderr, "\n");
    }
    exit(1);
}

//   from X11R6/programs/xwud.c 
Bool xwd::Read(char *ptr, int size, int nitems, FILE *stream) {
    size *= nitems;
    while (size) {
	nitems = fread(ptr, 1, size, stream);
	if (nitems <= 0)
	    return False;
	size -= nitems;
	ptr += nitems;
    }
    return True;
}

//   from X11R6/programs/xwud.c 
unsigned xwd::ImageSize() {
    if (image->format == ZPixmap)
        return((unsigned)image->bytes_per_line * image->height);
    else
        return(image->bytes_per_line * image->height * image->depth);
}

//   from X11R6/programs/xwud.c 
void xwd::_swapshort (char *bp, unsigned n) {
    register char c;
    register char *ep = bp + n;

    while (bp < ep) {
	c = *bp;
	*bp = *(bp + 1);
	bp++;
	*bp++ = c;
    }
}

//   from X11R6/programs/xwud.c 
void xwd::_swaplong (char *bp, unsigned n) {
    register char c;
    register char *ep = bp + n;
    register char *sp;

    while (bp < ep) {
	sp = bp + 3;
	c = *sp;
	*sp = *bp;
	*bp++ = c;
	sp = bp + 1;
	c = *sp;
	*sp = *bp;
	*bp++ = c;
	bp += 2;
    }
}




/*
Copyright (c) 1985, 1986, 1988  X Consortium

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from the X Consortium.

*/
