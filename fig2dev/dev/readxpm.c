/* START OF XPM SECTION TO LOOKUP NAMED COLORS */

#ifdef USE_XPM

/* The following code was lifted from oscolor.c in the X11 source code. */
/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

/* lookup the named colors referenced in the colortable passed */
/* total colors in the table are "ncols" */

#include <xpm.h>
#include <stdio.h>
#include "fig2dev.h"
#include "object.h"

#ifdef NDBM
#include <ndbm.h>
#else
#ifdef SVR4
#include <rpcsvc/dbm.h>
#else
#include <dbm.h>
#endif
#endif

#ifndef RGB_H
#define RGB_H
typedef struct _RGB {
	unsigned short red, green, blue;
	} RGB;
#endif /* RGB_H */

#ifdef NDBM
DBM *rgb_dbm = (DBM *)NULL;
#else
int rgb_dbm = 0;
#endif

convert_names(coltabl, ncols)
	XpmColor *coltabl;
	int	  ncols;
{
	int	i,j;
	char	*name;
	datum	dbent;
	RGB	rgb;

#ifdef NDBM
	rgb_dbm = dbm_open(RGB_FILE, 0, 0);
#else
	if (dbminit(RGB_FILE) == 0)
	    rgb_dbm = 1;
#endif
	if (!rgb_dbm) {
	    fprintf(stderr,"Couldn't open the RGB database file '%s'\n", RGB_FILE );
	    return;
	}
	/* look through each entry in the colortable for the named colors */
	for (i=0; i<ncols; i++) {
	    name = (coltabl+i)->c_color;
	    if (name[0]!='#') {		/* found named color, make lowercase */
	        for (j=strlen(name); j>=0; j--) {
		    if (isupper(name[j]))
			name[j]=tolower(name[j]);
		}
		dbent.dptr = name;
		dbent.dsize = strlen(name);
		/* look it up to get the rgb values */
#ifdef NDBM
		dbent = dbm_fetch(rgb_dbm, dbent);
#else
		dbent = fetch (dbent);
#endif
		if(dbent.dptr) {
			memcpy((char *) &rgb, dbent.dptr, sizeof (RGB));
		} else {
			fprintf(stderr,"can't parse color '%s', using black.\n",name);
			rgb.red=rgb.green=rgb.blue=0;
		}
		name = (coltabl+i)->c_color = (char *) malloc(7);
		/* change named color for #rrggbb type */
		sprintf(name,"#%.2x%.2x%.2x",rgb.red>>8, rgb.green>>8, rgb.blue>>8);
	    }
	}
}

#else /* USE_XPM */

/* for those compilers that don't like empty .c files */

static void
dum_function()
{
  ;
}

#endif /* USE_XPM */
