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
 * readtif.c: import tiff into PostScript
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "fig2dev.h"
#include "object.h"	/* does #include <X11/xpm.h> */
#include "messages.h"
#include "readpics.h"

extern	int	_read_pcx(FILE *pcxfile, F_pic *pic);	/* readpcx.c */

/* return codes:  1 : success
		  0 : invalid file
*/

int
read_tif(F_pic *pic, struct xfig_stream *restrict pic_stream, int *llx,int *lly)
{
	char	cmd_buf[128];
	char	*cmd = cmd_buf;
	char	cmd_fmt[] = "tifftopnm -quiet '%s' | ppmtopcx 2>/dev/null";
	FILE	*tiftopcx;
	int	 stat;

	/* tifftopnm needs a seekable file as input, no pipe.
	   See man tifftopnm(0). */
	if (uncompressed_content(pic_stream))
		return 0;
	*llx = *lly = 0;

	/* output PostScript comment */
	fprintf(tfp, "%% Originally from a TIFF File: %s\n\n", pic->file);

	/* allocate buffer and make command to convert tif to pnm then to pcx */
	if (sizeof cmd_fmt + strlen(pic_stream->content) - 2 > sizeof cmd_buf &&
			malloc(sizeof cmd_fmt + strlen(pic_stream->content) - 2)
			== NULL) {
		put_msg(Err_mem);
		return 0;
	}

	sprintf(cmd, cmd_fmt, pic_stream->content);

	if ((tiftopcx = popen(cmd, "r")) == NULL) {
		err_msg("Cannot open pipe to tifftopnm or ppmtopcx, %s", cmd);
		return 0;
	}
	if (cmd != cmd_buf)
		free(cmd);
	/* now call _read_pcx to read the pcx file */
	stat = _read_pcx(tiftopcx, pic);
	pic->transp = -1;
	/* close the pipe */
	pclose(tiftopcx);

	return stat;
}
