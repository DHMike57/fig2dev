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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "texfonts.h"

#include <stdio.h>
#include <math.h>		/* floor() */

#include "fig2dev.h"
#include "object.h"
#include "psfonts.h"		/* unpsfont() */


#ifdef NFSS
const char	*texfontfamily[] = {
	"\\familydefault",	/* default */
	"\\rmdefault",		/* roman */
	"\\rmdefault",		/* bold */
	"\\rmdefault",		/* italic */
	"\\sfdefault",		/* sans serif */
	"\\ttdefault"		/* typewriter */
};

const char	*texfontseries[] = {
	"\\mddefault",		/* default */
	"\\mddefault",		/* roman */
	"\\bfdefault",		/* bold */
	"\\mddefault",		/* italic */
	"\\mddefault",		/* sans serif */
	"\\mddefault"		/* typewriter */
};

const char	*texfontshape[] = {
	"\\updefault",		/* default */
	"\\updefault",		/* roman */
	"\\updefault",		/* bold */
	"\\itdefault",		/* italic */
	"\\updefault",		/* sans serif */
	"\\updefault"		/* typewriter */
};
const char	*texfonts[] = {
	"",			/* default */
	"\\rmfamily",		/* roman */
	"\\bfseries",		/* bold */
	"\\itshape",		/* italic */
	"\\sffamily",		/* sans serif */
	"\\ttfamily",		/* typewriter */
};
const char	*texpsfonts[] = {
	"{T1}{ptm}{m}{n}",	/* Times-Roman, default */
	"{T1}{ptm}{m}{n}",	/* Times-Roman */
	"{T1}{ptm}{m}{it}",	/* Times-Italic */
	"{T1}{ptm}{b}{n}",	/* Times-Bold */
	"{T1}{ptm}{b}{it}",	/* Times-BoldItalic */
	"{T1}{pag}{m}{n}",	/* AvantGarde */
	"{T1}{pag}{m}{sl}",	/* AvantGarde-BookOblique */
	"{T1}{pag}{db}{n}",	/* AvantGarde-Demi */
	"{T1}{pag}{db}{sl}",	/* AvantGarde-DemiOblique */
	"{T1}{pbk}{l}{n}",	/* Bookman-Light */
	"{T1}{pbk}{l}{it}",	/* Bookman-LightItalic */
	"{T1}{pbk}{db}{n}",	/* Bookman-Demi */
	"{T1}{pbk}{db}{it}",	/* Bookman-DemiItalic */
	"{T1}{pcr}{m}{n}",	/* Courier */
	"{T1}{pcr}{m}{sl}",	/* Courier-Oblique */
	"{T1}{pcr}{b}{n}",	/* Courier-Bold */
	"{T1}{pcr}{b}{sl}",	/* Courier-BoldItalic */
	"{T1}{phv}{m}{n}",	/* Helvetica */
	"{T1}{phv}{m}{sl}",	/* Helvetica-Oblique */
	"{T1}{phv}{b}{n}",	/* Helvetica-Bold */
	"{T1}{phv}{b}{sl}",	/* Helvetica-BoldOblique */
	"{T1}{phv}{mc}{n}",	/* Helvetica-Narrow */
	"{T1}{phv}{mc}{sl}",	/* Helvetica-Narrow-Oblique */
	"{T1}{phv}{bc}{n}",	/* Helvetica-Narrow-Bold */
	"{T1}{phv}{bc}{sl}",	/* Helvetica-Narrow-BoldOblique */
	"{T1}{pnc}{m}{n}",	/* NewCenturySchlbk-Roman */
	"{T1}{pnc}{m}{it}",	/* NewCenturySchlbk-Italic */
	"{T1}{pnc}{b}{n}",	/* NewCenturySchlbk-Bold */
	"{T1}{pnc}{b}{it}",	/* NewCenturySchlbk-BoldItalic */
	"{T1}{ppl}{m}{n}",	/* Palatino-Roman */
	"{T1}{ppl}{m}{it}",	/* Palatino-Italic */
	"{T1}{ppl}{b}{n}",	/* Palatino-Bold */
	"{T1}{ppl}{b}{it}",	/* Palatino-BoldItalic */
	"{U}{psy}{m}{n}",	/* Symbol */
	"{T1}{pzc}{mb}{it}",	/* ZapfChancery-MediumItalic */
	"{U}{pzd}{m}{n}"	/* ZapfDingbats */
};
#endif

const char	*texfontnames[] = {
	"rm",			/* default */
	"rm",			/* roman */
	"bf",			/* bold */
	"it",			/* italic */
	"sf",			/* sans serif */
	"tt"			/* typewriter */
};

/* The selection of font names may be site dependent.
 * Not all fonts are preloaded at all sizes.
 */
char	texfontsizes[] = {
	11,			/* default */
	5, 5, 5, 5,		/* 1-4: small fonts */
	5,			/* five point font */
	6, 7, 8,		/* etc */
	9, 10, 11,
	12, 12, 14,
	14, 14, 17,
	17, 17, 20,
	20, 20, 20, 20, 25,
	25, 25, 25, 29,
	29, 29, 29, 29,
	34, 34, 34, 34,
	34, 34, 34, 41,
	41, 41
};


/*
 * Output commands to select font and font size.
 */
void
select_font(F_text *t, bool select_fontsize, bool select_fontname,
			bool only_texfonts)
{
	int texsize, bprec;
	double baselineskip;

	/* tex-drivers by default use the correct font size, a bit different
	 * from the size on the screen */
	if (!select_fontsize) {
		if (!select_fontname)
			return;
	} else {
		texsize = round(t->size * (rigid_text(t) ? 1.0 : fontmag));
		baselineskip = (texsize * 1.2);
		if (baselineskip == floor(baselineskip))
			bprec = 0;
		else
			bprec = 1;

		fprintf(tfp, "\\fontsize{%d}{%.*f}",
					texsize, bprec, baselineskip);

		if (!select_fontname) {
			fputs("\\selectfont ", tfp);
			return;
		}
	}

	if (only_texfonts && psfont_text(t)) {
		unpsfont(t);
		t->flags -= PSFONT_TEXT;
	}

	if (psfont_text(t))
		fprintf(tfp, "\\usefont%s", texpsfonts[t->font <= MAX_PSFONT ?
				t->font + 1 : 0]);
	else
		/* Default psfont is -1, default texfont 0, also accept -1. */
		fprintf(tfp, "\\normalfont%s ", texfonts[t->font <= MAX_FONT ?
				(t->font >= 0 ? t->font : 0) : MAX_FONT - 1]);
}
