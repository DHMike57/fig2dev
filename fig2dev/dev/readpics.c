/*
 * TransFig: Facility for Translating Fig code
 *
 * Various copyrights in this file follow
 * Parts Copyright (c) 1994 Brian V. Smith
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

#define True 1
#define False 0

#include <stdio.h>
#include <sys/stat.h>
#include "fig2dev.h"
#include "object.h"

FILE	*open_picfile();
void	close_picfile();

/* define PATH_MAX if not already defined */
/* taken from the X11R5 server/os/osfonts.c file */
#ifndef X_NOT_POSIX
#ifdef _POSIX_SOURCE
#include <limits.h>
#else
#if !defined(sun) || defined(sparc)
#define _POSIX_SOURCE
#include <limits.h>
#undef _POSIX_SOURCE
#endif /* !defined(sun) || defined(sparc) */
#endif /* _POSIX_SOURCE */
#endif /* X_NOT_POSIX */

#ifndef PATH_MAX
#include <sys/param.h>
#ifdef MAXPATHLEN
#define PATH_MAX MAXPATHLEN
#else
#define PATH_MAX 1024
#endif /* MAXPATHLEN */
#endif /* PATH_MAX */

/*
   This is a modified version of the XReadBitmapFromFile() routine from
   the X11R5 distribution.  This version reads the XBM file into a (char*)
   array rather than creating the pixmap directly.
*/

/* $XConsortium: XRdBitF.c,v 1.15 91/02/01 16:34:46 gildea Exp $ */
/* Copyright, 1987, Massachusetts Institute of Technology */

/*
   Permission to use, copy, modify, distribute, and sell this software and its
   documentation for any purpose is hereby granted without fee, provided that
   the above copyright notice appear in all copies and that both that
   copyright notice and this permission notice appear in supporting
   documentation, and that the name of M.I.T. not be used in advertising or
   publicity pertaining to distribution of the software without specific,
   written prior permission.  M.I.T. makes no representations about the
   suitability of this software for any purpose.  It is provided "as is"
   without express or implied warranty.
*/

/*
 *	Code to read bitmaps from disk files. Interprets
 *	data from X10 and X11 bitmap files.
 *
 *	Modified for speedup by Jim Becker, changed image
 *	data parsing logic (removed some fscanf()s).
 *	Aug 5, 1988
 *
 * Note that this file and ../Xmu/RdBitF.c look very similar....  Keep them
 * that way (but don't use common source code so that people can have one
 * without the other).
 */


#define MAX_SIZE 255

/* shared data for the image read/parse logic */
static short hexTable[256];		/* conversion value */
static int initialized = 0;		/* easier to fill in at run time */


/*
 *	Table index for the hex values. Initialized once, first time.
 *	Used for translation value or delimiter significance lookup.
 */
static void initHexTable()
{
    /*
     * We build the table at run time for several reasons:
     *
     *     1.  portable to non-ASCII machines.
     *     2.  still reentrant since we set the init flag after setting table.
     *     3.  easier to extend.
     *     4.  less prone to bugs.
     */
    hexTable['0'] = 0;	hexTable['1'] = 1;
    hexTable['2'] = 2;	hexTable['3'] = 3;
    hexTable['4'] = 4;	hexTable['5'] = 5;
    hexTable['6'] = 6;	hexTable['7'] = 7;
    hexTable['8'] = 8;	hexTable['9'] = 9;
    hexTable['A'] = 10;	hexTable['B'] = 11;
    hexTable['C'] = 12;	hexTable['D'] = 13;
    hexTable['E'] = 14;	hexTable['F'] = 15;
    hexTable['a'] = 10;	hexTable['b'] = 11;
    hexTable['c'] = 12;	hexTable['d'] = 13;
    hexTable['e'] = 14;	hexTable['f'] = 15;

    /* delimiters of significance are flagged w/ negative value */
    hexTable[' '] = -1;	hexTable[','] = -1;
    hexTable['}'] = -1;	hexTable['\n'] = -1;
    hexTable['\t'] = -1;
	
    initialized = 1;
}

/*
 *	read next hex value in the input stream, return -1 if EOF
 */

static NextInt (fstream)
    FILE *fstream;
{
    int	ch;
    int	value = 0;
    int	ret_value = 0;
    int gotone = 0;
    int done = 0;

    /* loop, accumulate hex value until find delimiter  */
    /* skip any initial delimiters found in read stream */

    while (!done) {
	ch = getc(fstream);
	if (ch == EOF) {
	    value	= -1;
	    done++;
	} else {
	    /* trim high bits, check type and accumulate */
	    ch &= 0xff;
	    if (isascii(ch) && isxdigit(ch)) {
		value = (value << 4) + hexTable[ch];
		gotone++;
	    } else if ((hexTable[ch]) < 0 && gotone)
	      done++;
	}
    }

    ret_value = 0;
    if (value & 0x80)
	ret_value |= 0x01;
    if (value & 0x40)
	ret_value |= 0x02;
    if (value & 0x20)
	ret_value |= 0x04;
    if (value & 0x10)
	ret_value |= 0x08;
    if (value & 0x08)
	ret_value |= 0x10;
    if (value & 0x04)
	ret_value |= 0x20;
    if (value & 0x02)
	ret_value |= 0x40;
    if (value & 0x01)
	ret_value |= 0x80;
    return ret_value;

}

int ReadFromBitmapFile (filename, width, height, data_ret)
    char *filename;
    unsigned int *width, *height;       /* RETURNED */
    unsigned char **data_ret;           /* RETURNED */
{
    FILE	*fstream;		/* handle on file  */
    int		filtype;		/* file type (pipe or file) */
    unsigned	char *data = NULL;	/* working variable */
    char	line[MAX_SIZE];		/* input line from file */
    int		size;			/* number of bytes of data */
    char	name_and_type[MAX_SIZE]; /* an input line */
    char	*type;			/* for parsing */
    int		value;			/* from an input line */
    int		version10p;		/* boolean, old format */
    int		padding;		/* to handle alignment */
    int		bytes_per_line;		/* per scanline of data */
    unsigned	int ww = 0;		/* width */
    unsigned	int hh = 0;		/* height */

    /* first time initialization */
    if (initialized == 0)
	initHexTable();

    if ((fstream=open_picfile(filename, &filtype)) == NULL)
	    return 0;

    /* error cleanup and return macro	*/
#define	RETURN(code) { if (data) free (data); close_picfile(fstream,filtype); return code; }

    while (fgets(line, MAX_SIZE, fstream)) {
	if (strlen(line) == MAX_SIZE-1) {
	    RETURN (0);
	}
	if (sscanf(line,"#define %s %d",name_and_type,&value) == 2) {
	    if (!(type = rindex(name_and_type, '_')))
	      type = name_and_type;
	    else
	      type++;

	    if (!strcmp("width", type))
	      ww = (unsigned int) value;
	    if (!strcmp("height", type))
	      hh = (unsigned int) value;
	    continue;
	}

	if (sscanf(line, "static short %s = {", name_and_type) == 1)
	  version10p = 1;
	else if (sscanf(line,"static unsigned char %s = {",name_and_type) == 1)
	  version10p = 0;
	else if (sscanf(line, "static char %s = {", name_and_type) == 1)
	  version10p = 0;
	else
	  continue;

	if (!(type = rindex(name_and_type, '_')))
	  type = name_and_type;
	else
	  type++;

	if (strcmp("bits[]", type))
	  continue;

	if (!ww || !hh)
	  RETURN (0);

	if ((ww % 16) && ((ww % 16) < 9) && version10p)
	  padding = 1;
	else
	  padding = 0;

	bytes_per_line = (ww+7)/8 + padding;

	size = bytes_per_line * hh;
	data = (unsigned char *) malloc ((unsigned int) size);
	if (!data)
	  RETURN (0);

	if (version10p) {
	    unsigned char *ptr;
	    int bytes;

	    for (bytes=0, ptr=data; bytes<size; (bytes += 2)) {
		if ((value = NextInt(fstream)) < 0)
		  RETURN (0);
		*(ptr++) = value;
		if (!padding || ((bytes+2) % bytes_per_line))
		  *(ptr++) = value >> 8;
	    }
	} else {
	    unsigned char *ptr;
	    int bytes;

	    for (bytes=0, ptr=data; bytes<size; bytes++, ptr++) {
		if ((value = NextInt(fstream)) < 0)
		  RETURN (0);
		*ptr=value;
	    }
	}
    }					/* end while */

    if (data == NULL) {
	RETURN (0);
    }

    *data_ret = data;
    *width = ww;
    *height = hh;

    close_picfile(fstream, filtype);
    return (1);
}

/* END OF READ XBITMAP SECTION */



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

#include <X11/xpm.h>

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
			bcopy(dbent.dptr, (char *) &rgb, sizeof (RGB));
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

#endif USE_XPM

/* END OF XPM SECTION */



/* START OF GIF SECTION */

/*
 * FIG : Facility for Interactive Generation of figures
 * Parts Copyright 1990 David Koblas
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

/* The following code is extracted from giftoppm.c, from the pbmplus package */

/* +-------------------------------------------------------------------+ */
/* | Copyright 1990, David Koblas.                                     | */
/* |   Permission to use, copy, modify, and distribute this software   | */
/* |   and its documentation for any purpose and without fee is hereby | */
/* |   granted, provided that the above copyright notice appear in all | */
/* |   copies and that both that copyright notice and this permission  | */
/* |   notice appear in supporting documentation.  This software is    | */
/* |   provided "as is" without express or implied warranty.           | */
/* +-------------------------------------------------------------------+ */

#define	MAX_LWZ_BITS		12

#define INTERLACE		0x40
#define LOCALCOLORMAP		0x80
#define BitSet(byte, bit)	(((byte) & (bit)) == (bit))

#define	ReadOK(file,buffer,len)	(fread(buffer, len, 1, file) != 0)

#define LM_to_uint(a,b)			(((b)<<8)|(a))

struct {
	unsigned int	Width;
	unsigned int	Height;
	unsigned int	BitPixel;
	unsigned int	ColorResolution;
	unsigned int	Background;
	unsigned int	AspectRatio;
} GifScreen;

struct {
	int	transparent;
	int	delayTime;
	int	inputFlag;
	int	disposal;
} Gif89 = { -1, -1, -1, 0 };

static	int	verbose;
int	showComment;

static Boolean ReadColorMap();
static Boolean DoExtension();
static int GetDataBlock();
static int GetCode();
static int LWZReadByte();
static Boolean ReadGIFImage();

int
read_gif(pic)
  F_pic *pic;
{
	FILE		*fd;
	unsigned char	buf[16];
	unsigned char	c;
	int		useGlobalColormap;
	int		bitPixel;
	char		version[4];
	int		filtype;		/* file (0) or pipe (1) */

	if ((fd=open_picfile(pic->file, &filtype)) == NULL)
	    return 0;

	if (! ReadOK(fd,buf,6)) {
		close_picfile(fd,filtype);
		return 0;
	}

	if (strncmp(buf,"GIF",3) != 0) {
		close_picfile(fd,filtype);
		return -1;
	}

	strncpy(version, buf + 3, 3);
	version[3] = '\0';

	if ((strcmp(version, "87a") != 0) && (strcmp(version, "89a") != 0)) {
		fprintf(stderr,"Unknown GIF version %s\n",version);
		close_picfile(fd,filtype);
		return -1;
	}

	if (! ReadOK(fd,buf,7)) {
		close_picfile(fd,filtype);
		return 0;		/* failed to read screen descriptor */
	}

	GifScreen.Width           = LM_to_uint(buf[0],buf[1]);
	GifScreen.Height          = LM_to_uint(buf[2],buf[3]);
	GifScreen.BitPixel        = 2<<(buf[4]&0x07);
	GifScreen.ColorResolution = (((buf[4]&0x70)>>3)+1);
	GifScreen.Background      = buf[5];
	GifScreen.AspectRatio     = buf[6];

	if (BitSet(buf[4], LOCALCOLORMAP)) {	/* Global Colormap */
		if (!ReadColorMap(fd,GifScreen.BitPixel,pic->cmap)) {
			close_picfile(fd,filtype);
			return 0;		/* error reading global colormap */
		}
	}

	if (GifScreen.AspectRatio != 0 && GifScreen.AspectRatio != 49) {
		fprintf(stderr,"GIF: warning - non-square pixels\n");
	}

	for (;;) {
		if (! ReadOK(fd,&c,1)) {
			close_picfile(fd,filtype);	/* EOF / read error on image data */
			return 0;
		}

		if (c == ';') {		/* GIF terminator */
			close_picfile(fd,filtype);
			return 1;
		}

		if (c == '!') { 	/* Extension */
			if (! ReadOK(fd,&c,1))
				fprintf(stderr,"GIF read error on extention function code\n");
			(void) DoExtension(fd, c);
			continue;
		}

		if (c != ',') {		/* Not a valid start character */
			continue;
		}

		if (! ReadOK(fd,buf,9)) {
			close_picfile(fd,filtype);
			return 0;	/* couldn't read left/top/width/height */
		}

		useGlobalColormap = ! BitSet(buf[8], LOCALCOLORMAP);

		bitPixel = 1<<((buf[8]&0x07)+1);

		if (! useGlobalColormap) {
			if (!ReadColorMap(fd, bitPixel, pic->cmap)) {
				close_picfile(fd,filtype);
				fprintf(stderr,"error reading local GIF colormap\n" );
				return 0;
			}
			pic->numcols = bitPixel;
			if (!ReadGIFImage(pic, fd, LM_to_uint(buf[4],buf[5]),
				  LM_to_uint(buf[6],buf[7]),
				  BitSet(buf[8], INTERLACE)))
			    return 0;
		} else {
			if (!ReadGIFImage(pic, fd, LM_to_uint(buf[4],buf[5]),
				  LM_to_uint(buf[6],buf[7]),
				  BitSet(buf[8], INTERLACE)))
			    return 0;
			pic->numcols = GifScreen.BitPixel;
		}
	}
}

static Boolean
ReadColorMap(fd,number,cmap)
FILE	*fd;
int	number;
unsigned char cmap[3][MAXCOLORMAPSIZE];
{
	int		i;
	unsigned char	rgb[3];

	for (i = 0; i < number; ++i) {
		if (! ReadOK(fd, rgb, sizeof(rgb))) {
			fprintf(stderr,"bad GIF colormap\n" );
			return False;
		}
		cmap[0][i] = rgb[0];
		cmap[1][i] = rgb[1];
		cmap[2][i] = rgb[2];
	}
	return True;
}

static Boolean
DoExtension(fd, label)
FILE	*fd;
int	label;
{
	static char	buf[256];
	char		*str;

	switch (label) {
	case 0x01:		/* Plain Text Extension */
		str = "Plain Text Extension";
		break;
	case 0xff:		/* Application Extension */
		str = "Application Extension";
		break;
	case 0xfe:		/* Comment Extension */
		str = "Comment Extension";
		while (GetDataBlock(fd, (unsigned char*) buf) != 0) {
			; /* GIF comment */
		}
		return False;
	case 0xf9:		/* Graphic Control Extension */
		str = "Graphic Control Extension";
		(void) GetDataBlock(fd, (unsigned char*) buf);
		Gif89.disposal    = (buf[0] >> 2) & 0x7;
		Gif89.inputFlag   = (buf[0] >> 1) & 0x1;
		Gif89.delayTime   = LM_to_uint(buf[1],buf[2]);
		if ((buf[0] & 0x1) != 0)
			Gif89.transparent = buf[3];

		while (GetDataBlock(fd, (unsigned char*) buf) != 0)
			;
		return False;
	default:
		str = buf;
		sprintf(buf, "UNKNOWN (0x%02x)", label);
		break;
	}


	while (GetDataBlock(fd, (unsigned char*) buf) != 0)
		;

	return False;
}

int	ZeroDataBlock = False;

static int
GetDataBlock(fd, buf)
FILE		*fd;
unsigned char 	*buf;
{
	unsigned char	count;

	/* error in getting DataBlock size */
	if (! ReadOK(fd,&count,1)) {
		return -1;
	}

	ZeroDataBlock = count == 0;

	/* error in reading DataBlock */
	if ((count != 0) && (! ReadOK(fd, buf, count))) {
		return -1;
	}

	return count;
}

static int
GetCode(fd, code_size, flag)
FILE	*fd;
int	code_size;
int	flag;
{
	static unsigned char	buf[280];
	static int		curbit, lastbit, done, last_byte;
	int			i, j, ret;
	unsigned char		count;

	if (flag) {
		curbit = 0;
		lastbit = 0;
		done = False;
		return 0;
	}

	if ( (curbit+code_size) >= lastbit) {
		if (done) {
			/* if (curbit >= lastbit) then ran off the end of bits */
			return -1;
		}
		buf[0] = buf[last_byte-2];
		buf[1] = buf[last_byte-1];

		if ((count = GetDataBlock(fd, &buf[2])) == 0)
			done = True;

		last_byte = 2 + count;
		curbit = (curbit - lastbit) + 16;
		lastbit = (2+count)*8 ;
	}

	ret = 0;
	for (i = curbit, j = 0; j < code_size; ++i, ++j)
		ret |= ((buf[ i / 8 ] & (1 << (i % 8))) != 0) << j;

	curbit += code_size;

	return ret;
}

static int
LWZReadByte(fd, flag, input_code_size)
FILE	*fd;
int	flag;
int	input_code_size;
{
	static int	fresh = False;
	int		code, incode;
	static int	code_size, set_code_size;
	static int	max_code, max_code_size;
	static int	firstcode, oldcode;
	static int	clear_code, end_code;
	static int	table[2][(1<< MAX_LWZ_BITS)];
	static int	stack[(1<<(MAX_LWZ_BITS))*2], *sp;
	register int	i;

	if (flag) {
		set_code_size = input_code_size;
		code_size = set_code_size+1;
		clear_code = 1 << set_code_size ;
		end_code = clear_code + 1;
		max_code_size = 2*clear_code;
		max_code = clear_code+2;

		GetCode(fd, 0, True);
		
		fresh = True;

		for (i = 0; i < clear_code; ++i) {
			table[0][i] = 0;
			table[1][i] = i;
		}
		for (; i < (1<<MAX_LWZ_BITS); ++i)
			table[0][i] = table[1][0] = 0;

		sp = stack;

		return 0;
	} else if (fresh) {
		fresh = False;
		do {
			firstcode = oldcode =
				GetCode(fd, code_size, False);
		} while (firstcode == clear_code);
		return firstcode;
	}

	if (sp > stack)
		return *--sp;

	while ((code = GetCode(fd, code_size, False)) >= 0) {
		if (code == clear_code) {
			for (i = 0; i < clear_code; ++i) {
				table[0][i] = 0;
				table[1][i] = i;
			}
			for (; i < (1<<MAX_LWZ_BITS); ++i)
				table[0][i] = table[1][i] = 0;
			code_size = set_code_size+1;
			max_code_size = 2*clear_code;
			max_code = clear_code+2;
			sp = stack;
			firstcode = oldcode =
					GetCode(fd, code_size, False);
			return firstcode;
		} else if (code == end_code) {
			int		count;
			unsigned char	buf[260];

			if (ZeroDataBlock)
				return -2;

			while ((count = GetDataBlock(fd, buf)) > 0)
				;

			if (count != 0) {
				fprintf(stderr,"LWZReadByte: missing EOD in data stream (common occurence)\n");
			}
			return -2;
		}

		incode = code;

		if (code >= max_code) {
			*sp++ = firstcode;
			code = oldcode;
		}

		while (code >= clear_code) {
			*sp++ = table[1][code];
			if (code == table[0][code]) {
				fprintf(stderr,"LWZReadByte: circular table entry BIG ERROR\n");
			}
			code = table[0][code];
		}

		*sp++ = firstcode = table[1][code];

		if ((code = max_code) <(1<<MAX_LWZ_BITS)) {
			table[0][code] = oldcode;
			table[1][code] = firstcode;
			++max_code;
			if ((max_code >= max_code_size) &&
				(max_code_size < (1<<MAX_LWZ_BITS))) {
				max_code_size *= 2;
				++code_size;
			}
		}

		oldcode = incode;

		if (sp > stack)
			return *--sp;
	}
	return code;
}

static Boolean
ReadGIFImage(pic, fd, len, height, interlace)
F_pic	*pic;
FILE	*fd;
int	len, height;
int	interlace;
{
	unsigned char	c;	
	int		v;
	int		xpos = 0, ypos = 0, pass = 0;
	unsigned char	*image;

	/*
	**  Initialize the Compression routines
	*/
	if (! ReadOK(fd,&c,1))
		return False;		/* EOF / read error on image data */

	if (LWZReadByte(fd, True, c) < 0)
		return False;		/* error reading image */

	if ((image = (unsigned char*) malloc(len* height* sizeof(char))) == NULL)
		return False;		/* couldn't alloc space for image */

	while ((v = LWZReadByte(fd,False,c)) >= 0 ) {
		image[ypos*len+xpos] = (unsigned char) v;
		++xpos;
		if (xpos == len) {
			xpos = 0;
			if (interlace) {
				switch (pass) {
				case 0:
				case 1:
					ypos += 8; break;
				case 2:
					ypos += 4; break;
				case 3:
					ypos += 2; break;
				}

				if (ypos >= height) {
					++pass;
					switch (pass) {
					case 1:
						ypos = 4; break;
					case 2:
						ypos = 2; break;
					case 3:
						ypos = 1; break;
					default:
						goto fini;
					}
				}
			} else {
				++ypos;
			}
		}
		if (ypos >= height)
			break;
	}

fini:
	pic->subtype = P_GIF;
	pic->bitmap = image;	/* save the pixel data */
	pic->hw_ratio = (float) height / len;
	pic->bit_size.x = len;
	pic->bit_size.y = height;
	return True;
}

FILE *
open_picfile(name, type)
    char	*name;
    int		*type;
{
    char	unc[PATH_MAX+20];	/* temp buffer for uncompress/gunzip command */
    char	*compname;
    FILE	*fstream;		/* handle on file  */
    struct stat	status;

    *type = 0;
    compname = NULL;
    /* see if the filename ends with .Z */
    /* if so, generate uncompress command and use pipe (filetype = 1) */
    if (strlen(name) > 2 && !strcmp(".Z", name + (strlen(name)-2))) {
	sprintf(unc,"uncompress -c %s",name);
	*type = 1;
    /* or with .z or .gz */
    } else if ((strlen(name) > 3 && !strcmp(".gz", name + (strlen(name)-3))) ||
	      ((strlen(name) > 2 && !strcmp(".z", name + (strlen(name)-2))))) {
	sprintf(unc,"gunzip -qc %s",name);
	*type = 1;
    /* none of the above, see if the file with .Z or .gz or .z appended exists */
    } else {
	compname = (char*) malloc(strlen(name)+4);
	strcpy(compname, name);
	strcat(compname, ".Z");
	if (!stat(compname, &status)) {
	    sprintf(unc, "uncompress -c %s",compname);
	    *type = 1;
	    name = compname;
	} else {
	    strcpy(compname, name);
	    strcat(compname, ".z");
	    if (!stat(compname, &status)) {
		sprintf(unc, "gunzip -c %s",compname);
		*type = 1;
		name = compname;
	    } else {
		strcpy(compname, name);
		strcat(compname, ".gz");
		if (!stat(compname, &status)) {
		    sprintf(unc, "gunzip -c %s",compname);
		    *type = 1;
		    name = compname;
		}
	    }
	}
    }
    /* no appendages, just see if it exists */
    if (stat(name, &status) != 0) {
	fstream = NULL;
    } else {
	switch (*type) {
	  case 0:
	    fstream = fopen(name, "r");
	    break;
	  case 1:
	    fstream = popen(unc,"r");
	    break;
	}
    }
    if (compname)
	free(compname);
    return fstream;
}

void
close_picfile(file,type)
    FILE	*file;
    int		type;
{
    if (type == 0)
	fclose(file);
    else
	pclose(file);
}
