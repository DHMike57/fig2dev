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

extern struct driver dev_box;
extern struct driver dev_epic;
extern struct driver dev_ibmgl;
extern struct driver dev_latex;
extern struct driver dev_pic;
extern struct driver dev_pictex;
extern struct driver dev_ps;
extern struct driver dev_pstex;
extern struct driver dev_pstex_t;
extern struct driver dev_textyl;
extern struct driver dev_tk;
extern struct driver dev_tpic;
extern struct driver dev_mf;
extern struct driver dev_bitmaps;

/* all the bitmap formats use the dev_bitmaps driver */

struct 
	{char *name; struct driver *dev;}
	drivers[]
	= {
		{"acad",	&dev_bitmaps},
		{"box",		&dev_box}, 
		{"epic",	&dev_epic},
		{"eepic",	&dev_epic},
		{"eepicemu",	&dev_epic},
		{"gif",		&dev_bitmaps},
		{"ibmgl",	&dev_ibmgl},
		{"jpeg",	&dev_bitmaps},
		{"latex",	&dev_latex},
		{"mf",		&dev_mf},
		{"pcx",		&dev_bitmaps},
		{"pic",		&dev_pic},
		{"pictex",	&dev_pictex},
		{"png",		&dev_bitmaps},
		{"ppm",		&dev_bitmaps},
		{"ps",		&dev_ps},
		{"pstex",	&dev_pstex},
		{"pstex_t",	&dev_pstex_t},
		{"textyl",	&dev_textyl},
		{"tiff",	&dev_bitmaps},
		{"tk",		&dev_tk},
		{"tpic",	&dev_tpic},
		{"xbm",		&dev_bitmaps},
		{"xpm",		&dev_bitmaps},
		{"",		NULL}
	};
