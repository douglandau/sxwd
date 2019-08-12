/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2 -*-
   Copyright © 2001, 2004 Jamie Zawinski <jwz@jwz.org>

   bdf2c.c --- convert a BDF file to C source code.

   Permission to use, copy, modify, distribute, and sell this software and its
   documentation for any purpose is hereby granted without fee, provided that
   the above copyright notice appear in all copies and that both that
   copyright notice and this permission notice appear in supporting
   documentation.  No representations are made about the suitability of this
   software for any purpose.  It is provided "as is" without express or 
   implied warranty.
*/

#include "config.h"
#include "ppm-lite.h"
#include "font-bdf.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#undef countof
#define countof(x) (sizeof((x))/sizeof(*(x)))


const char *progname;


static void
write_ppm_src (int c, struct ppm *ppm, FILE *out)
{
  char buf [128];
  int col;
  unsigned const char *s = ppm->rgba;
  unsigned const char *end = ppm->rgba + (ppm->width * ppm->height * 4);

  sprintf (buf, "  { /* %c: %-3d */ %d, %d, %d, \"",
           (c == 0 || c == '\"' ? ' ' : c), c,
           ppm->type, ppm->width, ppm->height);
  col = strlen (buf);
  fprintf (out, "%s", buf);

  for (; s < end; s++)
    {
      fprintf (out, "\\%03o", *s);
      col += 4;
      if (col >= 72)
        {
          fprintf (out, "\"\n    \"");
          col = 5;
        }
    }

  fprintf (out, "\" },\n");
}

static void
write_bdf_src (const char *name, struct font *font, FILE *out)
{
  int i, j;
  fprintf (out, "struct font %s = {\n", name);
  fprintf (out, "  \"%s\",\n", font->name);
  fprintf (out, "  %d, %d, %d,\n",
           font->ascent, font->descent, font->monochrome_p);
  fprintf (out, "  {\n");
  for (i = 0, j = 0; i < countof(font->chars); i++)
    if (font->chars[i].ppm)
      {
        fprintf (out, "    { %3d, %3d, %3d, &%s_ppms[%d]\t/* %c: %-3d */ },\n",
                 font->chars[i].lbearing,
                 font->chars[i].width,
                 font->chars[i].descent,
                 name, j, ((i == 0 || i == '\"') ? ' ' : i), i);
        j++;
      }
    else
      {
        fprintf (out, "    { %3d, %3d, %3d, 0\t\t\t\t/*    %-3d */ },\n",
                 font->chars[i].lbearing,
                 font->chars[i].width,
                 font->chars[i].descent,
                 i);
      }

  fprintf (out, "  }\n");
  fprintf (out, "};\n");
}


static void
write_src (const char *name, struct font *font, const char *file,
           const char *infile)
{
  int stdout_p = !strcmp(file, "-");
  FILE *out;
  int i, j;
  int nppms;

  if (stdout_p)
    out = stdout;
  else
    {
      out = fopen (file, "w");
      if (!out)
        {
          char buf[1024];
          sprintf(buf, "%.255s: %.255s", progname, file);
          perror(buf);
          exit (1);
        }
    }

  nppms = 0;
  for (i = 0; i < countof(font->chars); i++)
    if (font->chars[i].ppm) nppms++;

  fprintf (out, "/* -*- Mode: C -*- */\n\n");
  fprintf (out, "/* GENERATED FILE -- DO NOT EDIT\n");
  fprintf (out, "   created by %s from %s\n  */\n\n",
           progname, infile);

  fprintf (out, "#ifdef __GNUC__\n  __extension__\n#endif\n\n");

  fprintf (out, "struct ppm %s_ppms[%d] = {\n", name, nppms);
  for (i = 0, j = 0; i < countof(font->chars); i++)
    {
      if (font->chars[i].ppm)
        {
          write_ppm_src (i, font->chars[i].ppm, out);
          j++;
        }
    }
  fprintf (out, "};\n\n");

  write_bdf_src (name, font, out);

  fprintf (out, "\n/* EOF */\n");

  if (!stdout_p)
    fclose (out);
  else
    fflush (out);
}




static void
usage (const char *msg, const char *arg)
{
  if (msg)
    {
      fprintf (stderr, "%s: ", progname);
      fprintf (stderr, msg, arg);
      fprintf (stderr, "\n\n");
    }

  fprintf (stderr, "usage: %s font-file [infile] [outfile]\n", progname);
  exit (1);
}

int
main (int argc, char **argv)
{
  char *s;
  int i;
  char *name = 0, *in = 0, *out = 0;
  struct font *font = 0;
  float scale = 0;
  int blur = 0;
  char dummy;

  progname = argv[0];
  s = strrchr(argv[0], '/');
  if (*s) progname = s+1;

  for (i = 1; i < argc; i++)
    {
      if (argv[i][0] == '-' && argv[i][1] == '-' && argv[i][2])
        argv[i]++;

      if (!strcmp (argv[i], "-scale"))
        {
          i++;
          if (1 != sscanf (argv[i], "%f %c", &scale, &dummy))
            usage("-scale \"%s\" unparsable (should be a float)", argv[i]);
        }
      else if (!strcmp (argv[i], "-blur"))
        {
          i++;
          if (1 != sscanf (argv[i], "%d %c", &blur, &dummy))
            usage("-blur \"%s\" unparsable (should be a number of pixels)",
                  argv[i]);
        }
      else if (argv[i][0] == '-')
        usage("unrecognised argument: `%s'", argv[i]);
      else if (!name)
        name = argv[i];
      else if (!in)
        in = argv[i];
      else if (!out)
        out = argv[i];
      else
        usage("unrecognised argument: `%s'", argv[i]);
    }

  if (!name) usage("no name specified", "");
  if (!in) in = "-";
  if (!out) out = "-";
  
  font = read_bdf (in);

  if (scale)
    scale_font (font, scale);

  if (blur)
    halo_font (font, blur);

  write_src (name, font, out, in);

  exit (0);
}
