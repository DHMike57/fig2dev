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

extern struct driver dev_box;
extern struct driver dev_cgm;
extern struct driver dev_epic;
extern struct driver dev_ibmgl;
extern struct driver dev_latex;
extern struct driver dev_pic;
extern struct driver dev_pictex;
extern struct driver dev_ps;
extern struct driver dev_eps;
extern struct driver dev_pdf;
extern struct driver dev_pstex;
extern struct driver dev_pstex_t;
extern struct driver dev_textyl;
extern struct driver dev_tk;
extern struct driver dev_tpic;
extern struct driver dev_mf;
extern struct driver dev_ge;
extern struct driver dev_bitmaps;
extern struct driver dev_map;

/* all the bitmap formats use the dev_bitmaps driver */

struct 
	{char *name; struct driver *dev;}
	drivers[] = {
		{"box",		&dev_box}, 
		{"cgm",		&dev_cgm}, 
		{"epic",	&dev_epic},
		{"eepic",	&dev_epic},
		{"eepicemu",	&dev_epic},
		{"ge",		&dev_ge},
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
		{"eps",		&dev_eps},
		{"pdf",		&dev_pdf},
		{"pstex",	&dev_pstex},
		{"pstex_t",	&dev_pstex_t},
		{"sld",		&dev_bitmaps},
		{"textyl",	&dev_textyl},
		{"tiff",	&dev_bitmaps},
		{"tk",		&dev_tk},
		{"tpic",	&dev_tpic},
		{"xbm",		&dev_bitmaps},
		{"xpm",		&dev_bitmaps},
		{"map",		&dev_map},
		{"",		NULL}
	};
