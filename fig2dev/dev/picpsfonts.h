/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1985 Supoj Sutantavibul
 * Copyright (c) 1991 Micah Beck
 * Copyright (c) 1992 Uri Blumenthal, IBM
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

#define              ULIMIT_FONT_SIZE        300
#define	       DEFAULT_PICFONT	     	13
#define PICPSFONT(F)      (PICfontnames[ \
		PICPSfontmap[(((F)->font) <= MAXFONT(F)) ? \
		((F)->font)+1 : \
		DEFAULT_PICFONT]])
extern int v2_flag, v21_flag;
#define ROMAN_DEFAULT 0
#define ROMAN 	1
#define ITALIC 	2
#define BOLD    3
#define ITABOL  4
#define HELVET  5
#define HELBOL  6
#define HELOBL  7
#define HELBOB  8
#define COUR    9
#define COURBL  10
#define COUROB  11
#define COURBO  12
#define SYMBOL  13
#define BRAKET  14
int    PICPSfontmap[] = {
			ROMAN_DEFAULT, ROMAN,
			ITALIC,
			BOLD,
			ITABOL,
			HELVET,
			HELOBL,
			HELBOL,
			HELBOB,
			ROMAN,
			ITALIC,
			BOLD,
			ITABOL,
			COUR,
			COUROB,
			COURBL,
			COURBO,
			HELVET,
			HELOBL,
			HELBOL,
			HELBOB,
			HELVET,
			HELOBL,
			HELBOL,
			HELBOB,
			ROMAN,
			ITALIC,
			BOLD,
			ITABOL,
			ROMAN,
			ITALIC,
			BOLD,
			ITABOL,
			SYMBOL,
			ITALIC,
			BRAKET
		};
char		*PICfontnames[] = {
		"R", "R",     /* default */
		"I",
		"B",
		"BI",
		"H",
		"HB",
		"HO",
		"HX",
		"C",
		"CB",
		"CO",
		"CX",
		"S",
		"S2"
		};
