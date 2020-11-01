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
 * readeps.c: import EPS into PostScript
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "fig2dev.h"	/* includes bool.h and object.h */
//#include "object.h"	/* includes X11/xpm.h */
#include "messages.h"
#include "readpics.h"

int	read_eps(F_pic *pic, struct xfig_stream *restrict pic_stream,
		int *llx, int *lly);

/* for both procedures:
     return codes:  1 : success
		    0 : failure
*/

/*
 * Scan a pdf-file for a /MediaBox specification. The FILE pointer file must
 * point to an open file-stream located close to the start of the file.
 * Return 0 on success, -1 on failure.
 */
static int
scan_mediabox(FILE *file, int *llx, int *lly, int *urx, int *ury)
{
	/*
	 * The line length of pdfs should not exceed 256 characters. However, in
	 * pdfs, the line end character might be a carriage return, while
	 * fgets() reads lines ended by newlines.
	 */
	char	buf[512];
	char	*s;
	int	ret = -1;	/* prime with failure */
	double	lx, ly, ux, uy;

	while (fgets(buf, sizeof buf, file) != NULL) {
		if ((s = strstr(buf, "/MediaBox"))) {
			s = strchr(s, '[');
			if (s && sscanf(s + 1, "%lf %lf %lf %lf",
						&lx, &ly, &ux, &uy) == 4) {
				*llx = (int)floor(lx);
				*lly = (int)floor(ly);
				*urx = (int)ceil(ux);
				*ury = (int)ceil(uy);
				ret = 0;
			}
			/* do not search for a second occurrence of /MediaBox */
			break;
		}
	}

	return ret;
}

/*
 * Read a PDF file.
 */
int
read_pdf(F_pic *pic, struct xfig_stream *restrict pic_stream, int *llx,int *lly)
{
	int	urx, ury;

	/*
	 * read_pdf() is called from genps.c, where the first 12 bytes were
	 * read. Take the risk, do not rewind, and continue to search for the
	 * /MediaBox.
	 *	if (!rewind_stream(pic_stream))
	 *		return 0;
	 */

	pic->subtype = P_EPS;

	if (scan_mediabox(pic_stream->fp, llx, lly, &urx, &ury)) {
		*llx = 0;
		*lly = 0;
		pic->bit_size.x = 10;
		pic->bit_size.y = 10;
	} else {
		pic->bit_size.x = urx - *llx;
		pic->bit_size.y = ury - *lly;
	}
	return 1;
}

/*
 * Read an EPS file.
 */
int
read_eps(F_pic *pic, struct xfig_stream *restrict pic_stream, int *llx,int *lly)
{
	char	buf[300];
	int	nested;

	if (!rewind_stream(pic_stream))
		return 0;

	pic->bit_size.x = pic->bit_size.y = 0;
	pic->subtype = P_EPS;

	/* give some initial values for bounding box in case none is found */
	*llx = 0;
	*lly = 0;
	pic->bit_size.x = 10;
	pic->bit_size.y = 10;

	/* scan for the bounding box */
	nested = 0;
	while (fgets(buf, sizeof buf, pic_stream->fp) != NULL) {
		if (!nested && !strncmp(buf, "%%BoundingBox:", 14)) {
			/* make sure doesn't say (atend) */
			if (!strstr(buf, "(atend)")) {
				double       rllx, rlly, rurx, rury;

				if (sscanf(strchr(buf, ':') + 1,
						"%lf %lf %lf %lf", &rllx, &rlly,
						&rurx,&rury) < 4) {
					put_msg("Bad EPS file: %s", pic->file);
					return 0;
				}
				*llx = floor(rllx);
				*lly = floor(rlly);
				pic->bit_size.x = (int)(rurx - rlly);
				pic->bit_size.y = (int)(rury - rlly);
				break;
			}
		} else if (!strncmp(buf, "%%Begin", 7)) {
			++nested;
		} else if (nested && !strncmp(buf, "%%End", 5)) {
			--nested;
		}
	}

	fprintf(tfp, "%% Begin Imported EPS File: %s\n", pic->file);
	fprintf(tfp, "%%%%BeginDocument: %s\n", pic->file);
	fputs("%\n", tfp);
	return 1;
}


/*
 * Write the eps-part of the epsi-file pointed to by in to out.
 * return codes:	 0	success,
 *			-1	nothing written,
 *		   termination	on partial write
 */
int
append_epsi(FILE *in, const char *filename, FILE *out)
{
	size_t		l = 12;
	unsigned char	buf[BUFSIZ];
	int		i;
	unsigned int	start = 0;
	unsigned int	length = 0;

	if (fread(buf, 1, l, in) != l) {
		fprintf(stderr, "Cannot read EPSI file %s.\n", filename);
		return -1;
	}
	for (i = 0; i < 4; ++i) {
		/* buf must be unsigned, otherwise the left shift fails */
		start += buf[i+4] << i*8;
		length += buf[i+8] << i*8;
	}
	/* read forward to start of eps section.
	   do not use fseek, in might be a pipe */
	if (fread(buf, 1, start - l, in) != start - l)
		return -1;

#define	EPSI_ERROR(mode)	fprintf(stderr, "Error when " mode \
		" embedded EPSI file %s.\nAborting.\n", filename), \
		exit(EXIT_FAILURE)

	l = BUFSIZ;
	while (length > l) {
		if (fread(buf, 1, l, in) != l)
			EPSI_ERROR("reading");
		length -= l;
		if (fwrite(buf, 1, l, out) != l)
			EPSI_ERROR("writing");
	}
	/* length < BUFSIZ  ( = l)*/
	if (length > 0) {
		if (fread(buf, 1, length, in) != length)
			EPSI_ERROR("reading");
		if (fwrite(buf, 1, length, out) != length)
			EPSI_ERROR("writing");
	}
	return 0;
}
