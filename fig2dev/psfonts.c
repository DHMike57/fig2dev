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
 *	psfont.c : PostScript font mappings
 *
*/
#include <stdio.h>
#include "fig2dev.h"
#include "object.h"

char			*PSfontnames[] = {
			"Times-Roman", "Times-Roman",	/* default */
			"Times-Italic",			/* italic */
			"Times-Bold",			/* bold */
			"Times-BoldItalic",
			"AvantGarde-Book",
			"AvantGarde-BookOblique",
			"AvantGarde-Demi",
			"AvantGarde-DemiOblique",
			"Bookman-Light",
			"Bookman-LightItalic",
			"Bookman-Demi",
			"Bookman-DemiItalic",
			"Courier",	
			"Courier-Oblique",
			"Courier-Bold",
			"Courier-BoldOblique",
			"Helvetica",
			"Helvetica-Oblique",
			"Helvetica-Bold",
			"Helvetica-BoldOblique",
			"Helvetica-Narrow",
			"Helvetica-Narrow-Oblique",
			"Helvetica-Narrow-Bold",
			"Helvetica-Narrow-BoldOblique",
			"NewCenturySchlbk-Roman",
			"NewCenturySchlbk-Italic",
			"NewCenturySchlbk-Bold",
			"NewCenturySchlbk-BoldItalic",
			"Palatino-Roman",
			"Palatino-Italic",
			"Palatino-Bold",
			"Palatino-BoldItalic",
			"Symbol",
			"ZapfChancery-MediumItalic",
			"ZapfDingbats"
		};

static int	PSfontmap[] = {
		ROMAN_FONT, ROMAN_FONT,		/* Times-Roman */
		ITALIC_FONT,			/* Times-Italic */
		BOLD_FONT,			/* Times-Bold */
		BOLD_FONT,			/* Times-BoldItalic */
		ROMAN_FONT,			/* AvantGarde */
		ROMAN_FONT,			/* AvantGarde-BookOblique */
		ROMAN_FONT,			/* AvantGarde-Demi */
		ROMAN_FONT,			/* AvantGarde-DemiOblique */
		ROMAN_FONT,			/* Bookman-Light */
		ITALIC_FONT,			/* Bookman-LightItalic */
		ROMAN_FONT,			/* Bookman-Demi */
		ITALIC_FONT,			/* Bookman-DemiItalic */
		TYPEWRITER_FONT,		/* Courier */
		TYPEWRITER_FONT,		/* Courier-Oblique */
		BOLD_FONT,			/* Courier-Bold */
		BOLD_FONT,			/* Courier-BoldItalic */
		MODERN_FONT,			/* Helvetica */
		MODERN_FONT,			/* Helvetica-Oblique */
		BOLD_FONT,			/* Helvetica-Bold */
		BOLD_FONT,			/* Helvetica-BoldOblique */
		MODERN_FONT,			/* Helvetica-Narrow */
		MODERN_FONT,			/* Helvetica-Narrow-Oblique */
		BOLD_FONT,			/* Helvetica-Narrow-Bold */
		BOLD_FONT,		/* Helvetica-Narrow-BoldOblique */
		ROMAN_FONT,			/* NewCenturySchlbk-Roman */
		ITALIC_FONT,			/* NewCenturySchlbk-Italic */
		BOLD_FONT,			/* NewCenturySchlbk-Bold */
		BOLD_FONT,		/* NewCenturySchlbk-BoldItalic */
		ROMAN_FONT,			/* Palatino-Roman */
		ITALIC_FONT,			/* Palatino-Italic */
		BOLD_FONT,			/* Palatino-Bold */
		BOLD_FONT,			/* Palatino-BoldItalic */
		ROMAN_FONT,			/* Symbol */
		ROMAN_FONT,			/* ZapfChancery-MediumItalic */
		ROMAN_FONT			/* ZapfDingbats */
		};

static int	PSmapwarn[] = {
		FALSE, FALSE,			/* Times-Roman */
		FALSE,				/* Times-Italic */
		FALSE,				/* Times-Bold */
		FALSE,				/* Times-BoldItalic */
		TRUE,				/* AvantGarde */
		TRUE,				/* AvantGarde-BookOblique */
		TRUE,				/* AvantGarde-Demi */
		TRUE,				/* AvantGarde-DemiOblique */
		TRUE,				/* Bookman-Light */
		TRUE,				/* Bookman-LightItalic */
		TRUE,				/* Bookman-Demi */
		TRUE,				/* Bookman-DemiItalic */
		FALSE,				/* Courier */
		TRUE,				/* Courier-Oblique */
		TRUE,				/* Courier-Bold */
		TRUE,				/* Courier-BoldItalic */
		FALSE,				/* Helvetica */
		TRUE,				/* Helvetica-Oblique */
		TRUE,				/* Helvetica-Bold */
		TRUE,				/* Helvetica-BoldOblique */
		TRUE,				/* Helvetica-Narrow */
		TRUE,				/* Helvetica-Narrow-Oblique */
		TRUE,				/* Helvetica-Narrow-Bold */
		TRUE,			/* Helvetica-Narrow-BoldOblique */
		TRUE,				/* NewCenturySchlbk-Roman */
		TRUE,				/* NewCenturySchlbk-Italic */
		TRUE,				/* NewCenturySchlbk-Bold */
		TRUE,			/* NewCenturySchlbk-BoldItalic */
		TRUE,				/* Palatino-Roman */
		TRUE,				/* Palatino-Italic */
		TRUE,				/* Palatino-Bold */
		TRUE,				/* Palatino-BoldItalic */
		TRUE,				/* Symbol */
		TRUE,				/* ZapfChancery-MediumItalic */
		TRUE				/* ZapfDingbats */
		};

int	        PSisomap[] = {
		FALSE, FALSE,			/* Times-Roman */
		FALSE,				/* Times-Italic */
		FALSE,				/* Times-Bold */
		FALSE,				/* Times-BoldItalic */
		FALSE,				/* AvantGarde */
		FALSE,				/* AvantGarde-BookOblique */
		FALSE,				/* AvantGarde-Demi */
		FALSE,				/* AvantGarde-DemiOblique */
		FALSE,				/* Bookman-Light */
		FALSE,				/* Bookman-LightItalic */
		FALSE,				/* Bookman-Demi */
		FALSE,				/* Bookman-DemiItalic */
		FALSE,				/* Courier */
		FALSE,				/* Courier-Oblique */
		FALSE,				/* Courier-Bold */
		FALSE,				/* Courier-BoldItalic */
		FALSE,				/* Helvetica */
		FALSE,				/* Helvetica-Oblique */
		FALSE,				/* Helvetica-Bold */
		FALSE,				/* Helvetica-BoldOblique */
		FALSE,				/* Helvetica-Narrow */
		FALSE,				/* Helvetica-Narrow-Oblique */
		FALSE,				/* Helvetica-Narrow-Bold */
		FALSE,			/* Helvetica-Narrow-BoldOblique */
		FALSE,				/* NewCenturySchlbk-Roman */
		FALSE,				/* NewCenturySchlbk-Italic */
		FALSE,				/* NewCenturySchlbk-Bold */
		FALSE,			/* NewCenturySchlbk-BoldItalic */
		FALSE,				/* Palatino-Roman */
		FALSE,				/* Palatino-Italic */
		FALSE,				/* Palatino-Bold */
		FALSE,				/* Palatino-BoldItalic */
		NO,				/* Symbol */
		FALSE,				/* ZapfChancery-MediumItalic */
		NO				/* ZapfDingbats */
		};

static char *figfontnames[] = {
		"Roman", "Roman",
		"Roman", 
		"Bold",
		"Italic",
		"Modern",
		"Typewriter"
		};

void unpsfont(t)
F_text	*t;
{
	if (!psfont_text(t)) return;
	if (PSmapwarn[t->font+1])
	  fprintf(stderr, "PS fonts not supported; substituting %s for %s\n",
		figfontnames[PSfontmap[t->font+1]+1], PSfontnames[t->font+1]);
	if (t->font == -1) /* leave default to be default, but no-ps */
	  t->font = 0;
	else
	  t->font = PSfontmap[t->font+1];
}

