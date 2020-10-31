/*
 * Fig2dev: Translate Fig code to various Devices
 * Copyright (c) 1991 by Micah Beck
 * Parts Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2015 by Brian V. Smith
 * Parts Copyright (c) 2015-2020 by Thomas Loimer
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

/*
 * readxpm.c: import xpm into PostScript
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#include "fig2dev.h"
#include "object.h"	/* does #include <X11/xpm.h> */
#include "readpics.h"

/* return codes:  1 : success
		  0 : invalid file
*/
int
read_xpm(F_pic *pic, struct xfig_stream *restrict pic_stream, int *llx,int *lly)
{

	if (uncompressed_content(pic_stream))
		return 0;
	*llx = *lly = 0;

	XpmReadFileToXpmImage(pic_stream->content, &pic->xpmimage, NULL);
	pic->subtype = P_XPM;
	pic->numcols = pic->xpmimage.ncolors;
	pic->bit_size.x = pic->xpmimage.width;
	pic->bit_size.y = pic->xpmimage.height;

	/* output PostScript comment */
	fprintf(tfp, "%% Begin Imported XPM File: %s\n\n", pic->file);
	return 1;
}
