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

#include <stdio.h>
#include "transfig.h"

/*
 * create an appropriate makefile
 */
makefile(mk, altfonts, arg_list)
FILE *mk;
int  altfonts;
argument *arg_list;
{
  argument *a;
  char *i;
  enum language to;
  int needps, needeps, needpic, needfig;
  
  fprintf(mk, "#\n# TransFig makefile\n#\n");

  fprintf(mk, "\nall: ");
  for (a = arglist; a; a = a->next)
    	fprintf(mk, "%s.tex ", a->name);
  fprintf(mk, "\n");

  for (a = arglist; a; a = a->next)
  {
	i = a->name;
	to = a->tolang;

	needps = needpic = needfig = 0;

	fprintf(mk, "\n# translation into %s\n\n", lname[(int)to]);

	switch (to)
	{
	case box:
		putfig(mk, box, altfonts, NULL, NULL, a->m, NULL, i, "tex");
		needfig = 1;
		break;

	case eepicemu:
	case epic:
	case eepic:
		putfig(mk, to, altfonts, a->f, a->s, a->m, a->o, i, "tex");
		needfig = 1;
		break;

	case latex:
		putfig(mk, latex, altfonts, a->f, a->s, a->m, a->o, i, "tex");
		needfig = 1;
		break;

	case pictex:
		putfig(mk, pictex, altfonts, a->f, a->s, a->m, a->o, i, "tex");
		needfig = 1;
		break;

	case postscript:
                puttarget(mk, i, "tex", "ps");
                fprintf(mk, "\tfig2ps2tex %s.ps >%s.tex\n", i, i);
		needps = 1;
                break;

	case encps:
                puttarget(mk, i, "tex", "eps");
                fprintf(mk, "\tfig2ps2tex %s.eps >%s.tex\n", i, i);
		needeps = 1;
                break;

        case psfig:
                puttarget(mk, i, "tex", "ps");
		fprintf(mk,"\techo '\\strut\\psfig{figure=%s.ps}' >%s.tex\n",
                          i, i);
		needps = 1;
                break;

	case pstex:

		/*
		 * The makefile for the pstex needs to update two files:
		 * file.eps which is created using fig2dev -L pstex file.fig
		 * and
		 * file.tex with fig2dev -L pstex_t -p file.eps file.fig
		 */
		puttarget(mk, i, "tex", "eps");
		fprintf(mk, "\tfig2dev -L pstex_t -p %s.eps ", i);
		putoptions(mk, altfonts, a->f, a->s, a->m, a->o, i, "tex");
		needps = 1;
		break;

	case textyl:
		putfig(mk, textyl, altfonts, a->f, a->s, a->m, a->o, i, "tex");
		needfig = 1;
		break;

	case tpic:
		puttarget(mk, i, "tex", "pic");
		/* fprintf(mk, "\ttpic %s.pic\n", i); */
		fprintf(mk, "\tpic2tpic %s.pic | tpic > %s.tex\n", i, i);
		fprintf(mk, "\techo \'\\strut\\box\\graph\' >>%s.tex\n", i);
		needpic = 1;
		break;

	}

	putclean(mk, i, "tex" );

	/* conversion to postscript */
	if (needps && a->type != ps) {
		if ( a->tops ) {
		    puttarget(mk, i, "ps", iname[(int)a->type]);
		    fprintf(mk, "\t%s %s.%s > %s.ps\n", a->tops, i, iname[(int)a->type], i);
		}
		else {
                    putfig(mk, (to == pstex ? pstex : postscript), 
			   altfonts, a->f, a->s, a->m, a->o, i, "ps");
                    a->interm = mksuff(i, ".ps");
		    needfig = 1;
		}
		putclean(mk, i, "ps" );
	}

	/* conversion to eps */
	if (needeps && a->type != eps) {
		if ( a->tops ) {
		    puttarget(mk, i, "eps", iname[(int)a->type]);
		    fprintf(mk, "\t%s %s.%s > %s.eps\n", a->tops, i, iname[(int)a->type], i);
		}
		else {
                    putfig(mk, (to == pstex ? pstex : encps), 
			   altfonts, a->f, a->s, a->m, a->o, i, "eps");
                    a->interm = mksuff(i, ".eps");
		    needfig = 1;
		}
		putclean(mk, i, "eps" );
	}

	/* conversion to pic */
	if (needpic && a->type != pic) {
		if ( a->topic ) {
		    puttarget(mk, i, "pic", iname[(int)a->type]);
		    fprintf(mk, "\t%s %s.%s > %s.pic\n", a->topic, i, iname[(int)a->type],i);
		}
		else {
		    putfig(mk, tpic, altfonts, a->f, a->s, a->m, a->o, i, "pic");

		    needfig = 1;
		}
		putclean(mk, i, "pic" );
	}

	/* conversion to fig */
	if (needfig && a->type != fig) {
		if ( a->tofig ) {
		    puttarget(mk, i, "fig", iname[(int)a->type]);
		    fprintf(mk, "\t%s %s.%s > %s.fig\n", a->tofig, i, iname[(int)a->type],i);
		    a->interm = mksuff(i, ".fig");
		}
		else {
		    fprintf(stderr, "transfig: warning: don't now how to make %s\n", mksuff(i, ".fig") );
		}
		putclean(mk, i, "fig" );
	}
  }
}

puttarget(mk, i, suf1, suf2)
FILE *mk;
char *i, *suf1, *suf2;
{
    fprintf(mk, "%s.%s: %s.%s %s\n", i, suf1, i, suf2, mkfile);
}

putfig(mk, to, altfonts, f, s, m, o, i, suf)
FILE *mk;
enum language to;
int altfonts;
char *f, *s, *m, *o, *i, *suf;
{
  fprintf(mk, "%s%s%s: %s.fig %s\n",
	       i, (suf ? "." : ""), (suf ? suf : ""), i, mkfile);

  if ( to == tpic )
	  fprintf(mk, "\tfig2dev -L pic ");
  else
	  fprintf(mk, "\tfig2dev -L %s ", lname[(int)to]);

  putoptions(mk, altfonts, f, s, m, o, i, suf);
}

putoptions(mk, altfonts, f, s, m, o, i, suf)
FILE *mk;
int altfonts;
char *f, *s, *m, *o, *i, *suf;
{
  if (altfonts==1) fprintf(mk, "-a ");
  if (f && *f) fprintf(mk, "-f %s ", f);
  if (s && *s) fprintf(mk, "-s %s ", s);
  if (m && *m) fprintf(mk, "-m %s ", m);
  if (o && *o) fprintf(mk, "%s ", o);

  fprintf(mk, "%s.fig > %s%s%s\n", i, i, (suf ? "." : ""), (suf ? suf : ""));
}

putclean(mk, i, suf)
FILE *mk;
char *i, *suf;
{
   fprintf(mk, "clean::\n");
   fprintf(mk, "\trm -f %s.%s\n", i, suf);
   fprintf(mk, "\n");
}
