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
 *	genpstex.c : psTeX and psTeX_t drivers for fig2dev
 *
 *	Author: Jose Alberto Fernandez R /Maryland CP 9/90
 * 	It uses the LaTeX and PostScript drivers to generate 
 *      LaTeX processed text for a Postscript figure.
 *
 * The pstex_t driver is like a latex driver that only translates 
 * text defined in the default font.
 *
 * The pstex driver is like a PostScript driver that translates 
 * everything except for text in the default font.
 *
 * The option '-p file' added to the pstex_t translator specifies
 * the name of the PostScript file to be called in the psfig macro.
 * If not set or its value is null then no PS file will be inserted.
 *
 * Jose Alberto.
 */

#if defined(hpux) || defined(SYSV) || defined(SVR4)
#include <sys/types.h>
#endif
#include <sys/file.h>
#include <stdio.h>
#include <math.h>
#include "fig2dev.h"
#include "object.h"
#include "texfonts.h"

#ifndef fabs
extern double fabs();
#endif
#ifndef sin
extern double sin();
#endif
#ifndef cos
extern double cos();
#endif
#ifndef acos
extern double acos();
#endif
#ifndef atan
extern double atan();
#endif
extern double rad2deg;

#ifdef hpux
#define rint(a) floor((a)+0.5)     /* close enough? */
#endif

#ifdef gould
#define rint(a) floor((a)+0.5)     /* close enough? */
#endif

extern void genlatex_start (),
	gendev_null (),
	genlatex_end (),
     	genps_option (),
	genps_start (),
	genps_arc (),
	genps_ellipse (),
	genps_line (),
	genps_spline (),
	genps_end (),
        genlatex_option (),
        genlatex_text (),
        genps_text ();

static char pstex_file[1000] = "";

void genpstex_t_option(opt, optarg)
char opt, *optarg;
{
       if (opt == 'p') strcpy(pstex_file, optarg);
       else genlatex_option(opt, optarg);
}


void genpstex_t_start(objects)
F_compound	*objects;
{
	/* Put PostScript Image if any*/
        if (pstex_file[0] != '\0')
        {
		fprintf(tfp, "\\begin{picture}(0,0)%%\n");
/* changed to use epsfig-macros April 13, 94 HGS*/
#ifdef EPSF
		fprintf(tfp, "\\epsfig{file=%s}%%\n",pstex_file); 
#else
		fprintf(tfp, "\\special{psfile=%s}%%\n",pstex_file);
#endif
		fprintf(tfp, "\\end{picture}%%\n");
	}
        genlatex_start(objects);

}

void genpstex_t_text(t)
F_text	*t;
{

	if (!special_text(t) && !psfont_text(t))
	  gendev_null(t);
	else genlatex_text(t);
}

void genpstex_text(t)
F_text	*t;
{

	if (!special_text(t) && !psfont_text(t))
	  genps_text(t);
	else gendev_null(t);
}

void genpstex_option(opt, optarg)
char opt, *optarg;
{
       if (opt != 'p') genlatex_option(opt, optarg);
}

struct driver dev_pstex_t = {
  	genpstex_t_option,
	genpstex_t_start,
	gendev_null,
	gendev_null,
	gendev_null,
	gendev_null,
	genpstex_t_text,
	genlatex_end,
	EXCLUDE_TEXT
};

struct driver dev_pstex = {
  	genps_option, /*HGS was genpstex_option, which complains about PS fonts*/
	genps_start,
	genps_arc,
	genps_ellipse,
	genps_line,
	genps_spline,
	genpstex_text,
	genps_end,
	INCLUDE_TEXT
};


