/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1985 Supoj Sutantavibul
 * Copyright (c) 1991 Micah Beck
 *
 * THE AUTHORS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * The X Consortium, and any party obtaining a copy of these files from
 * the X Consortium, directly or indirectly, is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and
 * documentation files (the "Software"), including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons who receive
 * copies from any such party to do so, with the only requirement being
 * that this copyright notice remain intact.  This license includes without
 * limitation a license to do the foregoing actions under any patents of
 * the party supplying this software to the X Consortium.
 */

/* 
 *	genbitmaps.c : bitmap driver for fig2dev 
 *
 *	Author: Brian V. Smith
 *		Handles ACAD, GIF, PCX, JPEG, TIFF, XBM and XPM.
 *		Uses genps functions to generate PostScript output then
 *		calls ghostscript to convert it to the output language
 *		if ghostscript has a driver for that language, or to ppm
 *		if ghostscript otherwise. If the latter, ppmtoxxx is then
 *		called to make the final XXX file.
 */

#if defined(hpux) || defined(SYSV) || defined(SVR4)
#include <sys/types.h>
#endif
#include <sys/file.h>
#include <stdio.h>
#include <math.h>
#include "fig2dev.h"
#include "object.h"
#include "texfonts.h"

extern void 
	genps_arc(),
	genps_ellipse(),
	genps_line(),
	genps_spline(),
	genps_text();

static char gscom[1000],*gsdev,tmpname[PATH_MAX];
Boolean	direct;
char	*ofile;
int	width,height;
int	jpeg_quality=75;
char	transparent[10]="\0";

void
genbitmaps_option(opt, optarg)
char opt;
char *optarg;
{
    switch (opt) {

	case 'q':			/* jpeg image quality */
	    if (strcmp(lang,"jpeg") != 0)
		fprintf(stderr,"-q option only allowed for jpeg quality; ignored\n");
	    sscanf(optarg,"%d",&jpeg_quality);
	    break;
	case 't':			/* GIF transparent color */
	    if (strcmp(lang,"gif") != 0)
		fprintf(stderr,"-t option only allowed for GIF transparent color; ignored\n");
	    (void) strcpy(transparent,optarg);
	    break;
	default:
	    break;
    }
}

void
genbitmaps_start(objects)
F_compound	*objects;
{

    char extra_options[200];
    float fscale;

    /* make small margin around in case bounding box errors */

    llx -= 200;
    ury += 200;

fscale = 14.0;

    /* make command for ghostscript */
    width=round(mag*(urx-llx)/fscale/*THICK_SCALE*/);
    height=round(mag*(ury-lly)/fscale/*THICK_SCALE*/);

/*
    width += 45;
    height += 13;
*/

    /* Add conditionals here if gs has a driver built-in */
    /* gs has a driver for png, ppm, pcx, jpeg and tiff */

    direct = TRUE;
    ofile = to;
    extra_options[0]='\0';

    if (strcmp(lang,"png")==0) {
	gsdev="png256";
    } else if (strcmp(lang,"pcx")==0) {
	gsdev="pcx256";
    } else if (strcmp(lang,"ppm")==0) {
	gsdev="ppmraw";
    } else if (strcmp(lang,"tiff")==0) {
	/* use the 24-bit - unfortunately, it doesn't use compression */
	gsdev="tiff24nc";
    } else if (strcmp(lang,"jpeg")==0) {
	gsdev="jpeg";
	/* set quality for JPEG */
	sprintf(extra_options,"-dJPEGQ=%d",jpeg_quality);
    } else {
	/* no driver in gs, use ppm output then use ppmtoxxx later */
	gsdev="ppmraw";
	/* make a unique name for the temporary ppm file */
	sprintf(tmpname,"f2d%d.ppm",getpid());
	ofile = tmpname;
	direct = FALSE;
    }
    /* make up the command for gs */
    sprintf(gscom, "gs -q -dSAFER -sDEVICE=%s -r80 -g%dx%d -sOutputFile=%s %s -",
		   gsdev,width,height,ofile,extra_options);
    /* divert output from ps driver to the pipe into ghostscript */
    /* but first close the output file that main() opened */
    fclose(tfp);
    if ((tfp = popen(gscom,"w" )) == 0) {
	fprintf(stderr,"Can't open pipe to ghostscript\n");
	exit(1);
    }
    genps_start(objects);
}

void
genbitmaps_end()
{
	char com[200];
	char *outfile;

	/* wrap up the postscript output */
	genps_end();
	/* add a showpage so ghostscript will produce output */
	fprintf(tfp, "showpage\n");

	if (pclose(tfp) != 0) {
	    fprintf(stderr,"Error in ghostcript command\n");
	    return;
	}
	/* for the formats that are only 8-bits, reduce the colors to 256 */
	/* and pipe through the ppm converter for that format */
	if (!direct) {
	    if (strcmp(lang, "gif")==0) {
		if (transparent[0]) {
		    /* escape the first char of the transparent color (#) for the shell */
		    sprintf(com,"(ppmquant 256 %s | ppmtogif -transparent \\%s",
			tmpname,transparent);
		} else {
		    sprintf(com,"(ppmquant 256 %s | ppmtogif",tmpname);
		}
	    } else if (strcmp(lang, "xbm")==0) {
		sprintf(com,"(ppmtopgm %s | pgmtopbm | pbmtoxbm",tmpname);
	    } else if (strcmp(lang, "xpm")==0) {
		sprintf(com,"(ppmquant 256 %s | ppmtoxpm",tmpname);
	    } else if (strcmp(lang, "acad")==0) {
		sprintf(com,"(ppmtoacad %s",tmpname);
	    }
	    if (tfp != stdout) {
		/* finally, route output from ppmtoxxx to final output file, if
		   not going to stdout */
		strcat(com," > ");
		strcat(com,to);
	    }
	    strcat(com,") 2> /dev/null");
	    /* execute the ppm program */
	    system(com);
	    /* finally, remove the temporary file */
	    unlink(tmpname);
	}
	/* we've already closed the original output file */
	tfp = 0;
}

struct driver dev_bitmaps = {
  	genbitmaps_option,
	genbitmaps_start,
	genps_arc,
	genps_ellipse,
	genps_line,
	genps_spline,
	genps_text,
	genbitmaps_end,
	INCLUDE_TEXT
};


