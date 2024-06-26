/*
 * Fig2dev: Translate Fig code to various Devices
 * Copyright (c) 1991 by Micah Beck
 * Parts Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2015 by Brian V. Smith
 * Parts Copyright (c) 2015-2023 by Thomas Loimer
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
 * psfont.c: PostScript font mappings
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include "bool.h"
#include "object.h"

char	*PSfontnames[] = {
		"Times-Roman",			/* default */
		"Times-Roman",
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

int	PSfontmap[] = {
		ROMAN_FONT, ROMAN_FONT,	/* Times-Roman */
		ITALIC_FONT,		/* Times-Italic */
		BOLD_FONT,		/* Times-Bold */
		BOLD_FONT,		/* Times-BoldItalic */
		ROMAN_FONT,		/* AvantGarde */
		ROMAN_FONT,		/* AvantGarde-BookOblique */
		ROMAN_FONT,		/* AvantGarde-Demi */
		ROMAN_FONT,		/* AvantGarde-DemiOblique */
		ROMAN_FONT,		/* Bookman-Light */
		ITALIC_FONT,		/* Bookman-LightItalic */
		ROMAN_FONT,		/* Bookman-Demi */
		ITALIC_FONT,		/* Bookman-DemiItalic */
		TYPEWRITER_FONT,	/* Courier */
		TYPEWRITER_FONT,	/* Courier-Oblique */
		BOLD_FONT,		/* Courier-Bold */
		BOLD_FONT,		/* Courier-BoldItalic */
		MODERN_FONT,		/* Helvetica */
		MODERN_FONT,		/* Helvetica-Oblique */
		BOLD_FONT,		/* Helvetica-Bold */
		BOLD_FONT,		/* Helvetica-BoldOblique */
		MODERN_FONT,		/* Helvetica-Narrow */
		MODERN_FONT,		/* Helvetica-Narrow-Oblique */
		BOLD_FONT,		/* Helvetica-Narrow-Bold */
		BOLD_FONT,		/* Helvetica-Narrow-BoldOblique */
		ROMAN_FONT,		/* NewCenturySchlbk-Roman */
		ITALIC_FONT,		/* NewCenturySchlbk-Italic */
		BOLD_FONT,		/* NewCenturySchlbk-Bold */
		BOLD_FONT,		/* NewCenturySchlbk-BoldItalic */
		ROMAN_FONT,		/* Palatino-Roman */
		ITALIC_FONT,		/* Palatino-Italic */
		BOLD_FONT,		/* Palatino-Bold */
		BOLD_FONT,		/* Palatino-BoldItalic */
		ROMAN_FONT,		/* Symbol */
		ROMAN_FONT,		/* ZapfChancery-MediumItalic */
		ROMAN_FONT		/* ZapfDingbats */
};

static int	PSmapwarn[] = {
		false, false,		/* Times-Roman */
		false,			/* Times-Italic */
		false,			/* Times-Bold */
		false,			/* Times-BoldItalic */
		true,			/* AvantGarde */
		true,			/* AvantGarde-BookOblique */
		true,			/* AvantGarde-Demi */
		true,			/* AvantGarde-DemiOblique */
		true,			/* Bookman-Light */
		true,			/* Bookman-LightItalic */
		true,			/* Bookman-Demi */
		true,			/* Bookman-DemiItalic */
		false,			/* Courier */
		true,			/* Courier-Oblique */
		true,			/* Courier-Bold */
		true,			/* Courier-BoldItalic */
		false,			/* Helvetica */
		true,			/* Helvetica-Oblique */
		true,			/* Helvetica-Bold */
		true,			/* Helvetica-BoldOblique */
		true,			/* Helvetica-Narrow */
		true,			/* Helvetica-Narrow-Oblique */
		true,			/* Helvetica-Narrow-Bold */
		true,			/* Helvetica-Narrow-BoldOblique */
		true,			/* NewCenturySchlbk-Roman */
		true,			/* NewCenturySchlbk-Italic */
		true,			/* NewCenturySchlbk-Bold */
		true,			/* NewCenturySchlbk-BoldItalic */
		true,			/* Palatino-Roman */
		true,			/* Palatino-Italic */
		true,			/* Palatino-Bold */
		true,			/* Palatino-BoldItalic */
		true,			/* Symbol */
		true,			/* ZapfChancery-MediumItalic */
		true			/* ZapfDingbats */
};

/* 0 for only ascii characters, 1 beyond ascii,
   2 for Symbol or Zapf Dingbats requiring latin1 encoding */
int	PSneedsutf8[] = {
		0, 0,			/* Times-Roman */
		0,			/* Times-Italic */
		0,			/* Times-Bold */
		0,			/* Times-BoldItalic */
		0,			/* AvantGarde */
		0,			/* AvantGarde-BookOblique */
		0,			/* AvantGarde-Demi */
		0,			/* AvantGarde-DemiOblique */
		0,			/* Bookman-Light */
		0,			/* Bookman-LightItalic */
		0,			/* Bookman-Demi */
		0,			/* Bookman-DemiItalic */
		0,			/* Courier */
		0,			/* Courier-Oblique */
		0,			/* Courier-Bold */
		0,			/* Courier-BoldItalic */
		0,			/* Helvetica */
		0,			/* Helvetica-Oblique */
		0,			/* Helvetica-Bold */
		0,			/* Helvetica-BoldOblique */
		0,			/* Helvetica-Narrow */
		0,			/* Helvetica-Narrow-Oblique */
		0,			/* Helvetica-Narrow-Bold */
		0,			/* Helvetica-Narrow-BoldOblique */
		0,			/* NewCenturySchlbk-Roman */
		0,			/* NewCenturySchlbk-Italic */
		0,			/* NewCenturySchlbk-Bold */
		0,			/* NewCenturySchlbk-BoldItalic */
		0,			/* Palatino-Roman */
		0,			/* Palatino-Italic */
		0,			/* Palatino-Bold */
		0,			/* Palatino-BoldItalic */
		2,			/* Symbol */
		0,			/* ZapfChancery-MediumItalic */
		2			/* ZapfDingbats */
};

static char *figfontnames[] = {
		"Roman", "Roman",
		"Roman",
		"Bold",
		"Italic",
		"Modern",
		"Typewriter"
};

void unpsfont(F_text *t)
{
	if (!psfont_text(t))
		return;
	if (PSmapwarn[t->font+1])
		fprintf(stderr,
			"PS fonts not supported; substituting %s for %s\n",
			figfontnames[PSfontmap[t->font+1]+1],
			PSfontnames[t->font+1]);
	if (t->font == -1) /* leave default to be default, but no-ps */
		t->font = 0;
	else
		t->font = PSfontmap[t->font+1];
}
