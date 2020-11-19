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
 * readppm.c: import ppm into PostScript
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdint.h>		/* INT16_MAX */
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <limits.h>

#include "fig2dev.h"	/* includes <object.h> */
//#include "object.h"	/* F_pic; does #include <X11/xpm.h> */
#include "messages.h"
#include "readpics.h"
#include "xtmpfile.h"

extern	int	_read_pcx(FILE *pcxfile, F_pic *pic);	/* readpcx.c */

/*
 * Return codes are mostly, but unfortunately not always,
 *	1: success,
 *	0: failure
 */


static int
skip_comments_whitespace(FILE *file)
{
	int	c;

	while ((c = fgetc(file)) != EOF) {
		if (c == '#')
			while ((c = fgetc(file)) != EOF && c != '\n')
				;
		if (c != ' ' && c != '\n' && c != '\r' && c != '\f' &&
				c != '\t' && c != '\v')
			break;
	}
	if (c == EOF) {
		return EOF;
	} else {
		ungetc(c, file);
		return 0;
	}
}

static int
read_8bitppm(FILE *file, unsigned char *restrict dst, unsigned int width,
						unsigned int height)
{
	int		c[3];
	unsigned int	w;

	while (height-- > 0u) {
		w = width;
		while (w-- > 0u) {
			if ((c[0] = fgetc(file)) == EOF ||
					(c[1] = fgetc(file)) == EOF ||
					(c[2] = fgetc(file)) == EOF)
				return 0;
			*(dst++) = (unsigned char)c[2];
			*(dst++) = (unsigned char)c[1];
			*(dst++) = (unsigned char)c[0];
		}
	}
	return 1;
}

static void
scale_to_255(unsigned char *restrict byte, unsigned maxval, unsigned rowbytes,
		unsigned height)
{
	unsigned int	w;
	const unsigned	rnd = maxval / 2;

	while (height-- > 0u) {
		w = rowbytes;
		while (w-- > 0u) {
			*byte = (*byte * 255u + rnd) / maxval;
			++byte;
		}
	}
}

/*
 * Read the next six bytes from file, which correspond to a rgb triplet with two
 * bytes for each channel, and return the values scaled into the range 0--255.
 */
static int
read6bytes(FILE *file, unsigned maxval, unsigned *r, unsigned *g, unsigned *b)
{
	int		c[6];
	const uint32_t	rnd = maxval / 2;

	if ((c[0] = fgetc(file)) == EOF || (c[1] = fgetc(file)) == EOF ||
		   (c[2] = fgetc(file)) == EOF || (c[3] = fgetc(file)) == EOF ||
		   (c[4] = fgetc(file)) == EOF || (c[5] = fgetc(file)) == EOF)
		return -1;

	/* scale to the range 0 - 255 */
	/* two-byte ppm files have the most significant byte first */
	*r = ((((uint32_t)c[0] << 8) + c[1]) * 255u + rnd) / maxval;
	*g = ((((uint32_t)c[2] << 8) + c[3]) * 255u + rnd) / maxval;
	*b = ((((uint32_t)c[4] << 8) + c[5]) * 255u + rnd) / maxval;

	if (*r > 255u)
		*r = 255u;
	if (*g > 255u)
		*g = 255u;
	if (*b > 255u)
		*b = 255u;

	return 0;
}

static int
read_16bitppm(FILE *file, unsigned char *restrict dst, unsigned int maxval,
		unsigned int width, unsigned int height)
{
	unsigned int	w;
	unsigned int	r, g, b;

	while (height-- > 0u) {
		w = width;
		while (w-- > 0u) {
			if (read6bytes(file, maxval, &r, &g, &b))
				return 0;

			*(dst++) = (unsigned char)b;
			*(dst++) = (unsigned char)g;
			*(dst++) = (unsigned char)r;
		}
	}
	return 1;
}

static int
read_asciippm(FILE *file, unsigned char *restrict dst, unsigned int width,
						unsigned int height)
{
	unsigned int	c[3];
	unsigned int	w;

	while (height-- > 0u) {
		w = width;
		while (w-- > 0u) {
			if (fscanf(file, " %u %u %u", c, c+1, c+2) != 3)
				return 0;

			if (c[0] > 255) c[0] = 255;
			if (c[1] > 255) c[1] = 255;
			if (c[2] > 255) c[2] = 255;

			*(dst++) = (unsigned char)c[2];
			*(dst++) = (unsigned char)c[1];
			*(dst++) = (unsigned char)c[0];
		}
	}
	return 1;
}

/*
 * Read a ppm file encoded with ascii decimal numbers, and scale to
 * the range 0--255. The function above, read_asciippm(), does not scale.
 */
static int
read_ascii_max_ppm(FILE *file, unsigned char *restrict dst, unsigned int maxval,
			unsigned int width, unsigned int height)
{
	uint32_t	c[3];
	unsigned int	w;
	const uint32_t	rnd = maxval / 2;

	while (height-- > 0u) {
		w = width;
		while (w-- > 0u) {
			if (fscanf(file, " %u %u %u", c, c+1, c+2) != 3)
				return 0;

			/* scale to the range 0--255 */
			c[0] = ((uint32_t)c[0] * 255u + rnd) / maxval;
			c[1] = ((uint32_t)c[1] * 255u + rnd) / maxval;
			c[2] = ((uint32_t)c[2] * 255u + rnd) / maxval;
			if (c[0] > 255) c[0] = 255;
			if (c[1] > 255) c[1] = 255;
			if (c[2] > 255) c[2] = 255;

			*(dst++) = (unsigned char)c[2];
			*(dst++) = (unsigned char)c[1];
			*(dst++) = (unsigned char)c[0];
		}
	}
	return 1;
}

static int
_read_ppm(FILE *file, F_pic *pic)
{
	int		c;
	int		magic;
	int		stat = 0;		/* prime with failure */
	unsigned int	height = 0u;
	unsigned int	width = 0u;
	unsigned int	rowbytes;
	unsigned int	maxval = 0u;

	/* get the magic number */
	if ((c = fgetc(file)) == EOF || c != 'P')
		return stat;
	if ((magic = fgetc(file)) == EOF || (magic != '6' && magic != '3'))
		return stat;

	if (skip_comments_whitespace(file))
		return stat;

	if (fscanf(file, "%u", &width) != 1)
		return stat;

	if (skip_comments_whitespace(file))
		return stat;

	if (fscanf(file, "%u", &height) != 1 || width == 0u || height == 0u)
		return stat;

	if (skip_comments_whitespace(file))
		return stat;

	if (fscanf(file, "%u", &maxval) != 1 || maxval > 65535u || maxval == 0u)
		return stat;

	if (skip_comments_whitespace(file))
		return stat;

	if (width > INT16_MAX || height > INT16_MAX) { /* large enough */
		fprintf(stderr, "fig2dev: PPM file %u x %u too large.\n",
				width, height);
		return stat;
	}

	rowbytes = width * 3u;
	pic->bitmap = malloc(height * rowbytes);
	if (pic->bitmap == NULL) {
		fputs("fig2dev: Out of memory, could not read PPM file.\n",
				stderr);
		return stat;
	}

	if (magic == '6') {
		if (maxval < 256u) {
			stat = read_8bitppm(file, pic->bitmap, width, height);
			if (maxval != 255u)
				scale_to_255(pic->bitmap, maxval, rowbytes,
						height);
		} else {
			stat = read_16bitppm(file, pic->bitmap, maxval,
					width, height);
		}
	} else { /* magic == '3' */
		if (maxval == 255u)
			stat = read_asciippm(file, pic->bitmap, width, height);
		else
			stat = read_ascii_max_ppm(file, pic->bitmap, maxval,
					width, height);
	}

	if (stat != 1) {
		free(pic->bitmap);
	} else {
		pic->subtype = P_PPM;
		pic->numcols = 2 << 24;
		pic->bit_size.x = width;
		pic->bit_size.y = height;
	}

	return stat;
}

/*
 * Read a ppm by first calling ppmtopcx via popen(). If ppmtopcx does not exist,
 * read the ppm in code, using _read_ppm(). Skip trying ImageMagick's convert or
 * gm convert, because these do not optimize image files. ppmtopcx counts colors
 * and, if possible, writes a file with a small color palette.
 * filetype: 0 - real file, 1 - pipe
 * Return: 0 failure, 1 success.
 */
int
read_ppm(F_pic *pic, struct xfig_stream *restrict pic_stream, int *llx,int *lly)
{
	int	stat;
	size_t	size;
	FILE	*f;
	char	buf[BUFSIZ];
	char	*cmd = buf;
	char	*const cmd_fmt = "ppmtopcx -quiet >%s 2>/dev/null";
	char	pcxname_buf[L_xtmpnam] = "f2dpcxXXXXXX";
	char	*pcxname = pcxname_buf;

	*llx = *lly = 0;

	if (!rewind_stream(pic_stream))
		return 0;

	/* make name for temp output file */
	if ((f = xtmpfile(&pcxname, sizeof pcxname_buf)) == NULL) {
		fprintf(stderr, "Cannot create temporary file %s\n", pcxname);
		if (pcxname != pcxname_buf)
			free(pcxname);
		return 0;
	}

	/* write the command for the pipe to cmd */
	size = sizeof cmd_fmt + strlen(pcxname) - 2;
	if (size > sizeof buf && (cmd = malloc(size)) == NULL) {
		put_msg(Err_mem);
		remove(pcxname);
		if (pcxname != pcxname_buf)
			free(pcxname);
		return 0;
	}
	if (sprintf(cmd, cmd_fmt, pcxname) < 0) {
		err_msg("fig2dev, I/O error");
		remove(pcxname);
		if (pcxname != pcxname_buf)
			free(pcxname);
		return 0;
	}

	/* pipe to ppmtopcx */
	if ((f = popen(cmd, "w"))) {
		while ((size=fread(buf, 1, sizeof buf, pic_stream->fp)) != 0)
			fwrite(buf, size, 1, f);

		/* close pipe */
		stat = pclose(f);
	} else {	/* f = NUll */
		remove(pcxname);
		stat = -1;
	}

	/* ppmtopcx succeeded */
	if (stat == 0) {
	       if ((f = fopen(pcxname, "rb"))) {
		       fprintf(tfp, "%% Originally from a PPM File: %s\n\n",
				       pic->file);
		       stat = _read_pcx(f, pic);
		       pic->transp = -1;
		       fclose(f);
	       } else {	/* f == NULL */
		       fprintf(stderr, "Cannot open temporary output file %s\n",
				       pcxname);
		       stat = -1;
	       }
	       remove(pcxname);
	}

	if (pcxname != pcxname_buf)
		free(pcxname);

	if (stat == 1) {
		return stat;
	} else {	/* ppmtopcx failed */
		f = rewind_stream(pic_stream);
		if (f == NULL)
			return stat;
		return _read_ppm(f, pic);
	}
}
