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

static char		*texfontnames[] = {
			"rm", "rm",		/* default */
			"rm",			/* roman */
			"bf",			/* bold */
			"it",			/* italic */
			"sf", 			/* sans serif */
			"tt"			/* typewriter */
		};

/* The selection of font names may be site dependent.
 * Not all fonts are preloaded at all sizes.
 */

static char		*texfontsizes[] = {
 			"Elv", "elv",		/* default */
 			"Fiv", "Fiv", "Fiv", "Fiv", 	/* small fonts */
 			"Fiv",			/* five point font */
 			"Six", "Sev", "Egt",	/* etc */
 			"Nin", "Ten", "Elv",
 			"Twl", "Twl", "Frtn",	
 			"Frtn", "Frtn", "Svtn",
 			"Svtn", "Svtn", "Twty",
 			"Twty", "Twty", "Twty", "Twty", "Twfv",
                        "Twfv", "Twfv", "twfv", "Twentynine",
                        "Twentynine", "Twentynine", "Twentynine", "Twentynine",
                        "Thirtyfour", "Thirtyfour", "Thirtyfour", "Thirtyfour",
                        "Thirtyfour", "Thirtyfour", "Thirtyfour", "Fortyone",
                        "Fortyone", "Fortyone"
  			};

#define MAXFONTSIZE 42

#define TEXFONT(F)	(texfontnames[((F) <= MAX_FONT) ? (F)+1 : MAX_FONT])
#define TEXFONTSIZE(S)	(texfontsizes[((S) <= MAXFONTSIZE) ? round(S)+1\
				      				: MAXFONTSIZE])
#define TEXFONTMAG(T)	TEXFONTSIZE(T->size*(rigid_text(T) ? 1.0 : mag))

