/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1985 Supoj Sutantavibul
 * Copyright (c) 1991 Micah Beck
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation. The authors make no representations about the suitability 
 * of this software for any purpose.  It is provided "as is" without express 
 * or implied warranty.
 *
 * THE AUTHORS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

#ifdef SYSV
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

