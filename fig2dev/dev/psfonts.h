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

static char		*psfontnames[] = {
			"Times-Roman", "Times-Roman",	/* default */
			"Times-Roman",			/* roman */
			"Times-Bold",			/* bold */
			"Times-Italic",			/* italic */
			"Helvetica",			/* sans serif */
			"Courier"			/* typewriter */
		};

extern int v2_flag, v21_flag, v30_flag;

#define PS_FONTNAMES(T)	\
  	(((v2_flag&&!(v21_flag||v30_flag)) || \
		psfont_text(T)) ? PSfontnames : psfontnames)

#define PSFONT(T) \
 ((T->font) <= MAXFONT(T) ? PS_FONTNAMES(T)[T->font+1] : PS_FONTNAMES(T)[0])

/* notice the conversion of resolution/80 for font sizes */

#define PSFONTMAG(T)	(((T->size) > 0 ? \
				((T->size) <= ULIMIT_FONT_SIZE ? \
			 		(T->size*resolution/80) : \
					ULIMIT_FONT_SIZE*resolution/80) : \
				font_size)/(rigid_text(T) ? mag : 1.0))
