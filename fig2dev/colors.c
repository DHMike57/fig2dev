/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1991 by Micah Beck
 * Parts Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-1999 by Brian V. Smith
 *
 * Any party obtaining a copy of these files is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and
 * documentation files (the "Software"), including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons who receive
 * copies from any such party to do so, with the only requirement being
 * that this copyright notice remain intact.
 *
 */

#include "fig2dev.h"

#ifdef NDBM
#include <ndbm.h>
#else
#ifdef SVR4
#include <rpcsvc/dbm.h>
#else
#include <dbm.h>
#endif
#endif

#ifdef NDBM
DBM *rgb_dbm = (DBM *)NULL;
#else
int rgb_dbm = 0;
#endif


/* initialize the X11 RGB color database file */

init_colordb()
{

#ifdef NDBM
	rgb_dbm = dbm_open(RGB_FILE, 0, 0);
#else
	if (dbminit(RGB_FILE) == 0)
	    rgb_dbm = 1;
#endif
	return 0;
}
	
int
lookup_db_color(name, rgb)
  char *name;
  RGB  *rgb;
{
    datum	dbent;
    int		j, n;
    int		r,g,b;
    char	rgb_txt[PATH_MAX];
    char	s[100], s1[100];
    FILE	*fp;

    if (name[0] == '#') {			/* hex color parse it now */
	if (strlen(name) == 4) {		/* #rgb */
		n = sscanf(name,"#%1x%1x%1x",&r,&g,&b);
		rgb->red   = ((r << 4) + r) << 8;
		rgb->green = ((g << 4) + g) << 8;
		rgb->blue  = ((b << 4) + b) << 8;
	} else if (strlen(name) == 7) {		/* #rrggbb */
		n = sscanf(name,"#%2x%2x%2x",&r,&g,&b);
		rgb->red   = r << 8;
		rgb->green = g << 8;
		rgb->blue  = b << 8;
	} else if (strlen(name) == 10) {	/* #rrrgggbbb */
		n = sscanf(name,"#%3x%3x%3x",&r,&g,&b);
		rgb->red   = r << 4;
		rgb->green = g << 4;
		rgb->blue  = b << 4;
	} else if (strlen(name) == 13) {	/* #rrrrggggbbbb */
		n = sscanf(name,"#%4x%4x%4x",&r,&g,&b);
		rgb->red   = r;
		rgb->green = g;
		rgb->blue  = b;
	}
	if (n != 3) {
	    rgb->red=rgb->green=rgb->blue=0;
	    return -1;
	}
    } else {
	/* make colorname all lower case */
	for (j=strlen(name); j>=0; j--) {
	    if (isupper(name[j]))
		name[j]=tolower(name[j]);
	}
	if (rgb_dbm) {
	    dbent.dptr = name;
	    dbent.dsize = strlen(name);
	    /* look it up to get the rgb values */
#ifdef NDBM
	    dbent = dbm_fetch(rgb_dbm, dbent);
#else
	    dbent = fetch (dbent);
#endif
	    if(dbent.dptr) {
		memcpy((char *) rgb, dbent.dptr, sizeof (RGB));
	    } else {
		rgb->red=rgb->green=rgb->blue=0;
		return -1;
	    }
	} else {  /* no DBM file;  try to read rgb.txt */
	    sprintf(rgb_txt, "%s.txt", RGB_FILE);
	    fp = fopen(rgb_txt, "r");
	    if (fp == NULL) {
	      fprintf(stderr,"Couldn't open the RGB database file '%s'\n", rgb_txt);
	      return -1;
	    } else {
	      while (fgets(s, sizeof(s), fp)) {
		if (sscanf(s, "%d %d %d %s", &r, &g, &b, s1) == 4) {
		  if (strcasecmp(name, s1) == 0) {
		    rgb->red = r << 8;
		    rgb->green = g << 8;
		    rgb->blue = b << 8;
		    fclose(fp);
		    return 0;
		  }
		}
	      }
	      fclose(fp);
	      rgb->red=rgb->green=rgb->blue=0;
	      return -1;
	    }
	}
    }
    return 0;
}
