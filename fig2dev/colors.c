/*
 * Fig2dev: Translate Fig code to various Devices
 * Copyright (c) 1991 by Micah Beck
 * Parts Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2002 by Brian V. Smith
 * Parts Copyright (c) 2015-2017 by Thomas Loimer
 *
 * Any party obtaining a copy of these files is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and documentation
 * files (the "Software"), including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense and/or sell copies
 * of the Software, and to permit persons who receive copies from any such
 * party to do so, with the only requirement being that the above copyright
 * and this permission notice remain intact.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef	HAVE_STRINGS_H
#include <strings.h>
#endif
#include "pi.h"

#include "fig2dev.h"	/* includes "bool.h" */

struct color_db {
	char		*name;
	unsigned short	red, green, blue;
};

static int		read_colordb(void);
static int		numXcolors = 0;
static struct color_db	*Xcolors;


static void
strip_blank(char *dest, const char *src)
{
	while (*src)
		if (*src == ' ' || *src == '\t')
			++src;
		else
			*dest++ = *src++;
	*dest = '\0';
}

int
lookup_X_color(char *name, RGB *rgb)
{
	static bool	have_read_X_colors = false;
	size_t		len;
	struct color_db *col;

	len = strlen(name);
	if (name[0] == '#') {			/* hex color parse it now */
	    unsigned short	r, g, b;
	    int			n = 0;
	    if (len == 4) {		/* #rgb */
		    n = sscanf(name,"#%1hx%1hx%1hx",&r,&g,&b);
		    rgb->red   = ((r << 4) + r) << 8;
		    rgb->green = ((g << 4) + g) << 8;
		    rgb->blue  = ((b << 4) + b) << 8;
	    } else if (len == 7) {		/* #rrggbb */
		    n = sscanf(name,"#%2hx%2hx%2hx",&r,&g,&b);
		    rgb->red   = r << 8;
		    rgb->green = g << 8;
		    rgb->blue  = b << 8;
	    } else if (len == 10) {	/* #rrrgggbbb */
		    n = sscanf(name,"#%3hx%3hx%3hx",&r,&g,&b);
		    rgb->red   = r << 4;
		    rgb->green = g << 4;
		    rgb->blue  = b << 4;
	    } else if (len == 13) {	/* #rrrrggggbbbb */
		    n = sscanf(name,"#%4hx%4hx%4hx",&r,&g,&b);
		    rgb->red   = r;
		    rgb->green = g;
		    rgb->blue  = b;
	    }
	    if (n == 3) {
		/* ok */
		return 0;
	    }
	} else {
	    /* read the X color database file if we haven't done that yet */
	    if (!have_read_X_colors) {
		if (read_colordb() != 0) {
		    /* error reading color database, return black */
		    rgb->red = rgb->green = rgb->blue = 0u;
		    return -1;
		}
		have_read_X_colors = true;
	    }

	    /* named color, look in the database we read in */
	    for (col = Xcolors; col - Xcolors < numXcolors; ++col) {
		char	buf[100];
		char	*strip_name;

		strip_name = len >= 100 ? malloc(len + 1) : buf;
		strip_blank(strip_name, name);
		if (strcasecmp(col->name, strip_name) == 0) {
		    /* found it */
		    rgb->red = col->red;
		    rgb->green = col->green;
		    rgb->blue = col->blue;
		    return 0;
		}
	    }
	}
	/* not found or bad #rgb spec, set to black */
	rgb->red = rgb->green = rgb->blue = 0;
	return -1;
}

/* read the X11 RGB color database (ASCII .txt) file */

int
read_colordb(void)
{
    static int		maxcolors = 400;
    FILE		*fp;
    char		s[100], s1[100];
    unsigned short	r,g,b;
    struct color_db	*col;

    fp = fopen(RGB_FILE, "r");
    if (fp == NULL) {
      fprintf(stderr,"Couldn't open the RGB database file '%s'\n", RGB_FILE);
      return -1;
    }
    if ((Xcolors = (struct color_db*) malloc(maxcolors*sizeof(struct color_db)))
		== NULL) {
      fprintf(stderr,"Couldn't allocate space for the RGB database file\n");
      return -1;
    }

    while (fgets(s, sizeof(s), fp)) {
	if (numXcolors >= maxcolors) {
	    maxcolors += 500;
	    if ((Xcolors = (struct color_db*) realloc(Xcolors, maxcolors*sizeof(struct color_db))) == NULL) {
	      fprintf(stderr,"Couldn't allocate space for the RGB database file\n");
	      return -1;
	    }
	}
	if (sscanf(s, "%hu %hu %hu %[^\n]", &r, &g, &b, s1) == 4) {
	    char	*c1, *c2;
	    /* remove any white space from the color name */
	    for (c1=s1, c2=s1; *c2; ++c2) {
		if (*c2 != ' ' && *c2 != '\t') {
		   *c1 = *c2;
		   ++c1;
		}
	    }
	    *c1 = '\0';
	    col = (Xcolors+numXcolors);
	    col->red = r << 8;
	    col->green = g << 8;
	    col->blue = b << 8;
	    col->name = malloc((size_t)(c1 - s1 + 1));
	    strcpy(col->name, s1);
	    ++numXcolors;
	}
    }
    fclose(fp);
    return 0;
}

/* convert rgb to gray scale using the classic luminance conversion factors */

float
rgb2luminance (float r, float g, float b)
{
      return (float) (0.3*r + 0.59*g + 0.11*b);
}
