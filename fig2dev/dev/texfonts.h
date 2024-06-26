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

#ifndef TEXFONTS_H
#define TEXFONTS_H

#if defined HAVE_CONFIG_H && !defined VERSION
#include "config.h"
#endif

#include "bool.h"
#include "object.h"

#ifdef NFSS
extern const char	*texfontfamily[];
extern const char	*texfontseries[];
extern const char	*texfontshape[];
extern const char	*texfonts[];
extern const char	*texpsfonts[];
#endif

extern const char	*texfontnames[];
extern char		texfontsizes[];

#define MAXFONTSIZE	42

#ifdef NFSS
#define TEXFAMILY(F)	texfontfamily[(F) <= MAX_FONT ? ((F) >= 0 ? (F) : 0) \
						: MAX_FONT-1]
#define TEXSERIES(F)	texfontseries[(F) <= MAX_FONT ? ((F) >= 0 ? (F) : 0) \
						: MAX_FONT-1]
#define TEXSHAPE(F)	texfontshape[(F) <= MAX_FONT ? ((F) >= 0 ? (F) : 0) \
						: MAX_FONT-1]
#endif
#define TEXFONT(F)	texfontnames[(F) <= MAX_FONT ? ((F) >= 0 ? (F) : 0) \
						: MAX_FONT-1]

/*
#define TEXFONTSIZE(S)	(texfontsizes[((S) <= MAXFONTSIZE) ? (int)(round(S))\
							: (MAXFONTSIZE-1)])
*/
#define TEXFONTSIZE(S)	((S) <= MAXFONTSIZE ? texfontsizes[(int)round(S)] : (S))
#define TEXFONTMAG(T)	TEXFONTSIZE(T->size*(rigid_text(T) ? 1.0 : fontmag))

extern void	put_string(char *restrict string, int font, bool tex_text);
extern void	select_font(F_text *t, bool select_fontsize,
				bool select_fontname, bool only_texfonts);

#endif /* TEXFONTS_H */
