/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1989-1999 by Brian V. Smith
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
#include "object.h"

extern	int	_read_pcx();

/* return codes:  1 : success
		  0 : invalid file
*/

int
read_tif(filename,filetype,pic,llx,lly)
    char	   *filename;
    int		    filetype;
    F_pic	   *pic;
    int		   *llx, *lly;
{
	char	 buf[2*PATH_MAX+40],pcxname[PATH_MAX];
	FILE	*giftopcx;
	int	 stat, size;

	*llx = *lly = 0;
	/* output PostScript comment */
	fprintf(tfp, "%% Originally from a TIFF File: %s\n\n", pic->file);

	/* make name for temp output file */
	sprintf(pcxname, "%s/%s%06d.pix", TMPDIR, "xfig-pcx", getpid());
	/* make command to convert tif to pnm then to pcx into temp file */
	sprintf(buf, "tifftopnm %s 2> /dev/null | ppmtopcx > %s 2> /dev/null",
		filename, pcxname);
	if ((giftopcx = popen(buf,"w" )) == 0) {
	    fprintf(stderr,"Cannot open pipe to tifftopnm or ppmtopcx\n");
	    /* remove temp file */
	    unlink(pcxname);
	    return 0;
	}
	/* close pipe */
	pclose(giftopcx);
	if ((giftopcx = fopen(pcxname, "r")) == NULL) {
	    fprintf(stderr,"Can't open temp output file\n");
	    /* remove temp file */
	    unlink(pcxname);
	    return 0;
	}
	/* now call _read_pcx to read the pcx file */
	stat = _read_pcx(giftopcx, pic);
	/* remove temp file */
	unlink(pcxname);

	return stat;
}

