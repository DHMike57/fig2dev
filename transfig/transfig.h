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

#if defined(SYSV) || defined(SVR4)
#include <string.h>
#else
#include <strings.h>
#define	strchr	index
#define	strrchr	rindex
#endif

/*
 * converters program names
 */
#define FIG2DEV	"fig2dev"
#define PIC2FIG "pic2fig"
#define APG2FIG "apgto f"

/*
 * filename defaults
 */
#define MK "Makefile"
#define TX "transfig.tex"

enum language  {box, epic, eepic, eepicemu, latex,
	pictex, postscript, psfig, pstex, textyl, tpic};
#define MAXLANG tpic

enum input {apg, fig, pic, ps};
#define MAXINPUT xps

typedef struct argument{
	char *name, *interm, *f, *s, *m, *o, *tofig, *topic, *tops;
	enum language tolang;
	enum input type;
	struct argument *next;
} argument ;

extern enum language str2lang();
extern char *lname[];
extern char *iname[];

extern char *sysls(), *mksuff();
extern argument *arglist;
extern char *txfile, *mkfile;

extern char *optarg;
extern int optind;

