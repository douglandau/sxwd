/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2 -*-
   Copyright © 2001, 2002, 2004 Jamie Zawinski <jwz@jwz.org>

   ppmcaption.c --- command-line processing

   Permission to use, copy, modify, distribute, and sell this software and its
   documentation for any purpose is hereby granted without fee, provided that
   the above copyright notice appear in all copies and that both that
   copyright notice and this permission notice appear in supporting
   documentation.  No representations are made about the suitability of this
   software for any purpose.  It is provided "as is" without express or 
   implied warranty.

   Usage:
     ppmcaption -font ncenB24.bdf -scale 0.34 -blur 3 \
        -pos  '10 -10' -left  -text 'The DNA Lounge' \
        -pos '-10 -10' -right -text '%a, %d-%b-%Y %l:%M:%S %p %Z' \
      infile outfile
*/

#include "config.h"
#include "ppm-lite.h"
#include "font-bdf.h"

#ifdef BUILTIN_FONT
# include "builtin.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>

#undef countof
#define countof(x) (sizeof((x))/sizeof(*(x)))

const char *progname;

static void
usage (const char *msg, const char *arg)
{
  if (msg)
    {
      fprintf (stderr, "%s: ", progname);
      fprintf (stderr, msg, arg);
      fprintf (stderr, "\n");
    }

  fprintf (stderr, "usage: %s [options] [infile] [outfile]\n", progname);

  if (msg)
    fprintf (stderr, "try `%s -help' for detailed option descriptions.\n",
             progname);
  else
    fprintf (stderr,

# ifdef __GNUC__
  __extension__    /* shut up about "string length is greater than the length
                      ISO C89 compilers are required to support" when including
                      the .ad file... */
# endif

    "\n"
    "Input and output files are PBM, PGM, or PPM files.\n"
    "Any file name may be \"-\", meaning stdin or stdout.\n"
    "\n"
    "Options may occur multiple times, and are processed in order.\n"
    "The options are:\n"
    "\n"
    "   --font <filename>   Specifies a BDF font file to load.\n"
#ifdef BUILTIN_FONT
    "                       The default is to use a builtin font.\n"
#else
    "                       Since no default font was built in at\n"
    "                       compile-time, this must be specified before\n"
    "                       the first `-text' argument.\n"
#endif
    "\n"
    "   --scale <float>     Scale the currently-selected font up or down.\n"
    "\n"
    "   --blur <integer>    Add an N pixel halo around the currently\n"
    "                       selected font, so that it is visible on both\n"
    "                       light and dark backgrounds.\n"
    "\n"
    "   --opacity <float>   How transparent to draw the next block of text.\n"
    "                       0.0 means invisible, 1.0 means solid.\n"
    "\n"
    "   --fg <color>        Foreground color of next block of text.\n"
    "   --bg <color>        Background color (color of blur halo).\n"
    "                       A small number of color names are supported\n"
    "                       (\"black\", \"white\", etc.) or hexadecimal\n"
    "                       triplets of the form \"#RRGGBB\" may be used.\n"
    "\n"
    "   --pos <X> <Y>       Where to position the next block of text.\n"
    "                       Positive numbers: from the upper left;\n"
    "                       Negative numbers: from the bottom right.\n"
    "\n"
    "   --left              The next block of text will have its left edge\n"
    "                        at the current position (this is the default.)\n"
    "   --right             The next block of text will have its right edge\n"
    "                        at the current position.\n"
    "   --center            The next block of text will be centered on top\n"
    "                        of the current position.  Note that this only\n"
    "                        applies to the X position, not Y: multi-line\n"
    "                        text is also centered on X, but grows down.\n"
    "\n"
    "   --text <string>     Place the given text in the image at the current\n"
    "                       position and in the current font.  Newlines are\n"
    "                       allowed; tabs are not handled.  If the text\n"
    "                       contains percent signs (%%) then they will be\n"
    "                       interpreted as per strftime(3) and date(1).\n"
    "                       double them (%%%%) to insert a literal %%.\n"
    "\n");
  exit (1);
}


static struct { const char *name; unsigned long hex; }
color_names[] = {
  { "black",   0x000000 },
  { "silver",  0xC0C0C0 },
  { "gray",    0x808080 },
  { "white",   0xFFFFFF },
  { "maroon",  0x800000 },
  { "red",     0xFF0000 },
  { "purple",  0x800080 },
  { "fuchsia", 0xFF00FF },
  { "green",   0x008000 },
  { "lime",    0x00FF00 },
  { "olive",   0x808000 },
  { "yellow",  0xFFFF00 },
  { "navy",    0x000080 },
  { "blue",    0x0000FF },
  { "teal",    0x008080 },
  { "aqua",    0x00FFFF }
};


static unsigned long
parse_color (const char *color)
{
  const char *s;
  int i;

  for (i = 0; i < countof(color_names); i++)
    {
      if (!strcasecmp (color, color_names[i].name))
        return color_names[i].hex;
    }

  if (color[0] == '#') color++;
  s = color;
  if (strlen(s) != 6) goto FAIL;
  for (; *s; s++)
    if (! ((*s >= '0' && *s <= '9') ||
           (*s >= 'A' && *s <= 'F') ||
           (*s >= 'a' && *s <= 'f')))
      {
      FAIL:
        usage ("unparsable color name \"%s\": try #RRGGBB", color);
      }

# define DEHEX(N) (((N) >= '0' && (N) <= '9') ? (N)-'0' : \
                   ((N) >= 'A' && (N) <= 'F') ? (N)-'A'+10 : (N)-'a'+10)
  s = color;
  return (((DEHEX(s[0]) << 4 | DEHEX(s[1])) << 16) |
          ((DEHEX(s[2]) << 4 | DEHEX(s[3])) << 8) |
          ((DEHEX(s[4]) << 4 | DEHEX(s[5]))));
# undef DEHEX
}


int
main (int argc, char **argv)
{
  char *s;
  char dummy;
  char *in = 0, *out = 0;
  int i;
  struct ppm *ppm = 0;
  struct font *font = 0;
  int pos_p = 0;
  int pos_x = 0;
  int pos_y = 0;
  int opacity = 255;
  int alignment = 1;
  int verbose = 0;
  unsigned long fg = 0x000000;
  unsigned long bg = 0xFFFFFF;
  
  char **cmds = (char **) calloc (sizeof(*cmds), argc);
  int ncmds = 0;
  int saw_text = 0;

  time_t now = time ((time_t *) 0);

  progname = argv[0];
  s = strrchr(argv[0], '/');
  if (s) progname = s+1;

  for (i = 1; i < argc; i++)
    {
      if (argv[i][0] == '-' && argv[i][1] == '-' && argv[i][2])
        argv[i]++;
          
      if (!strcmp (argv[i], "-help"))
        usage(0, 0);
      else if (!strcmp (argv[i], "-font") ||
               !strcmp (argv[i], "-scale") ||
               !strcmp (argv[i], "-blur") ||
               !strcmp (argv[i], "-opacity") ||
               !strcmp (argv[i], "-fg") ||
               !strcmp (argv[i], "-bg") ||
               !strcmp (argv[i], "-time") ||
               !strcmp (argv[i], "-text"))
        {
          if (!strcmp (argv[i], "-text"))
            saw_text = 1;

          if (!argv[i+1] || argv[i+1][0] == '-')
            usage("argument required for `%s'", argv[i]);

          if (verbose > 1)
            fprintf (stderr, "%s: parse: \"%s\" \"%s\"\n",
                     progname, argv[i], argv[i+1]);

          cmds[ncmds++] = argv[i++];
          cmds[ncmds++] = argv[i];
        }
      else if (!strcmp (argv[i], "-left") ||
               !strcmp (argv[i], "-right") ||
               !strcmp (argv[i], "-center"))
        {
          cmds[ncmds++] = argv[i];
          if (verbose > 1)
            fprintf (stderr, "%s: parse: \"%s\"\n", progname, argv[i]);
        }
      else if (!strcmp (argv[i], "-pos"))
        {
          /* Allow:
             { "-pos", "x", "y" }
             { "-pos", "x y" }
             { "-pos", "x,y" }
           */
          int x, y;
          if (!argv[i+1])
            usage("argument required for `%s'", argv[i]);

          cmds[ncmds++] = argv[i++];

          while ((s = strchr(argv[i], ',')))
            *s = ' ';
          while ((s = strchr(argv[i], '/')))
            *s = ' ';

          if (2 == sscanf (argv[i], " %d %d %c", &x, &y, &dummy)) /* "x y" */
            {
              if (verbose > 1)
                fprintf (stderr, "%s: parse: \"%s\" \"%s\"\n",
                         progname, cmds[ncmds-1], argv[i]);
              cmds[ncmds++] = argv[i];
            }
          else if (1 == sscanf (argv[i],   " %d %c", &x, &dummy) && /* "x" */
                   1 == sscanf (argv[i+1], " %d %c", &y, &dummy))   /* "y" */
            {
              char *s = (char *)
                malloc(strlen(argv[i]) + strlen(argv[i+1]) + 4);
              strcpy (s, argv[i++]);
              strcat (s, " ");
              strcat (s, argv[i]);
              if (verbose > 1)
                fprintf (stderr,
                         "%s: parse: \"%s\" \"%s\" \"%s\" ==> \"%s\"\n",
                         progname, cmds[ncmds-1], argv[i-1], argv[i], s);
              cmds[ncmds++] = strdup (s);
            }
          else
            usage("-pos \"%s\" unparsable (should be \"X Y\")", argv[i]);
        }
      else if (!strcmp (argv[i], "-verbose") || !strcmp (argv[i], "-v"))
        verbose++;
      else if (!strcmp (argv[i], "-vv"))
        verbose += 2;
      else if (argv[i][0] == '-' && argv[i][1])
        usage("unrecognised argument: `%s'", argv[i]);
      else if (!in)
        in = argv[i];
      else if (!out)
        out = argv[i];
      else
        usage("unrecognised argument: `%s'", argv[i]);
    }

  if (ncmds == 0)
    usage("no commands specified.", "");
  else if (!saw_text)
    usage("no -text specified.", "");


  if (!in) in = "-";
  if (!out) out = "-";
  
  ppm = read_ppm (in);

  for (i = 0; i < ncmds; i++)
    {
      if (!strcmp (cmds[i], "-font"))
        {
          i++;
          if (font) free_font (font);

          if (verbose)
            fprintf (stderr, "%s: loading font %s\n", progname, cmds[i]);

#ifdef BUILTIN_FONT
          if (!strcmp(cmds[i], "builtin"))
            font = copy_font (&builtin_font);
          else
#endif
            font = read_bdf (cmds[i]);
        }
      else if (!strcmp (cmds[i], "-pos"))
        {
          i++;
          if (2 != sscanf (cmds[i], "%d %d %c", &pos_x, &pos_y, &dummy))
            usage("-pos \"%s\" unparsable (should be \"X Y\")", cmds[i]);

          if (pos_x < 0) pos_x = ppm->width  + pos_x;
          if (pos_y < 0) pos_y = ppm->height + pos_y;
          pos_p = 1;

          if (verbose)
            fprintf (stderr, "%s: position: %d %d\n", progname, pos_x, pos_y);
        }
      else if (!strcmp (cmds[i], "-scale"))
        {
          float scale;
          i++;
          if (1 != sscanf (cmds[i], "%f %c", &scale, &dummy))
            usage("-scale \"%s\" unparsable (should be a float)", cmds[i]);

#ifdef BUILTIN_FONT
          if (!font) font = copy_font (&builtin_font);
#endif

          if (!font) usage ("-font must preceed -scale", "");

          if (verbose)
            fprintf (stderr, "%s: scale: %.2f\n", progname, scale);

          scale_font (font, scale);
        }
      else if (!strcmp (cmds[i], "-blur"))
        {
          int b;
          i++;
          if (1 != sscanf (cmds[i], "%d %c", &b, &dummy))
            usage("-blur \"%s\" unparsable (should be a number of pixels)",
                  cmds[i]);

          if (b > 0)
            {
#ifdef BUILTIN_FONT
              if (!font) font = copy_font (&builtin_font);
#endif
              if (!font) usage ("-font must preceed -blur", "");

              if (verbose)
                fprintf (stderr, "%s: blur: %d\n", progname, b);

              halo_font (font, b);
            }
        }
      else if (!strcmp (cmds[i], "-fg"))
        {
          i++;
          fg = parse_color (cmds[i]);
          if (verbose)
            fprintf (stderr, "%s: fg: 0x%06lX\n", progname, fg);
        }
      else if (!strcmp (cmds[i], "-bg"))
        {
          i++;
          bg = parse_color (cmds[i]);

          if (verbose)
            fprintf (stderr, "%s: bg: 0x%06lX\n", progname, bg);
        }
      else if (!strcmp (cmds[i], "-time"))
        {
          unsigned long tt;
          i++;
          if (!strcmp (cmds[i], "file"))
            {
              char *file = in;
              struct stat st;
              int ok = 0;
              if (!strcmp (in, "-"))
                {
                  file = "<stdin>";
                  ok = !fstat (fileno (stdin), &st);
                }
              else
                {
                  ok = !stat (in, &st);
                }

              if (!ok)
                {
                  char buf [1024];
                  sprintf(buf, "%s: unable to stat %s", progname, file);
                  perror(buf);
                  exit (1);
                }

              now = st.st_mtime;
            }
          else if (1 == sscanf (cmds[i], "%lu %c", &tt, &dummy) && tt > 0)
            now = (time_t) tt;
          else
            usage("-time \"%s\" unparsable (should be \"file\" or a time_t)",
                  cmds[i]);
        }
      else if (!strcmp (cmds[i], "-opacity"))
        {
          float op;
          i++;
          if (1 != sscanf (cmds[i], "%f %c", &op, &dummy) ||
              op <= 0.0 || op > 1.0)
            usage ("-opacity \"%s\" unparsable (should be a float (0, 1]",
                   cmds[i]);

          if (verbose)
            fprintf (stderr, "%s: opacity: %.2f\n", progname, op);

          opacity = 255 * op;
        }
      else if (!strcmp (cmds[i], "-left"))
        {
          alignment = 1;
          if (verbose)
            fprintf (stderr, "%s: alignment: left\n", progname);
        }
      else if (!strcmp (cmds[i], "-right"))
        {
          alignment = -1;
          if (verbose)
            fprintf (stderr, "%s: alignment: right\n", progname);
        }
      else if (!strcmp (cmds[i], "-center"))
        {
          alignment = 0;
          if (verbose)
            fprintf (stderr, "%s: alignment: center\n", progname);
        }
      else if (!strcmp (cmds[i], "-text"))
        {
          char *text = cmds[++i];
#ifdef BUILTIN_FONT
          if (!font) font = copy_font (&builtin_font);
#endif
          if (!font) usage ("-font must preceed -text", "");

          if (!pos_p) usage ("-pos must preceed -text", "");

          if (strchr (text, '%'))
            {
              struct tm *tm = localtime (&now);
              int L = strlen(text) + 100;
              char *t2 = (char *) malloc (L);
              strftime (t2, L-1, text, tm);
              text = t2;
            }

          if (verbose)
            fprintf (stderr, "%s: text: \"%s\"\n", progname, text);

          draw_string (font, (unsigned char *) text, ppm,
                       pos_x,
                       pos_y - font->ascent,
                       alignment, fg, bg, opacity);
        }
      else
        abort();
    }

  if (verbose)
    fprintf (stderr, "%s: writing: %s\n", progname, out);

  write_ppm (ppm, out);
  exit (0);
}
