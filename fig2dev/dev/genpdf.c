/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1989-1999 by Brian V. Smith
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

/* 
 *	genpdf.c : pdf driver for fig2dev 
 *
 *	Author: Brian V. Smith
 *		Uses genps functions to generate PostScript output then
 *		calls ghostscript (device pdfwrite) to convert it to pdf.
 */

#if defined(hpux) || defined(SYSV) || defined(SVR4)
#include <sys/types.h>
#endif
#include <sys/file.h>
#include <signal.h>
#include "fig2dev.h"
#include "genps.h"
#include "object.h"
#include "texfonts.h"

static	Boolean	 direct;
static	FILE	*saveofile;
static	char	*ofile;
static	char	 tempfile[PATH_MAX];

void
genpdf_option(opt, optarg)
char opt;
char *optarg;
{
	/* just use the ps options */
	pdfflag = True;
	epsflag = True;
	gen_ps_eps_option(opt, optarg);
}

void
genpdf_start(objects)
F_compound	*objects;
{
    /* divert output from ps driver to the pipe into ghostscript */
    /* but first close the output file that main() opened */
    saveofile = tfp;
    if (tfp != stdout)
	fclose(tfp);

    /* make up the command for gs */
    ofile = (to == NULL? "-": to);
    sprintf(gscom,
	 "gs -q -dNOPAUSE -sAutoRotatePages=None -sDEVICE=pdfwrite -sOutputFile=%s - -c quit",
		ofile);
    (void) signal(SIGPIPE, gs_broken_pipe);
    if ((tfp = popen(gscom,"w" )) == 0) {
	fprintf(stderr,"fig2dev: Can't open pipe to ghostscript\n");
	fprintf(stderr,"command was: %s\n", gscom);
	exit(1);
    }
    genps_start(objects);
}

int
genpdf_end()
{
	char	 com[PATH_MAX*2],tempofile[PATH_MAX];
	FILE	*tmpfile;
	int	 status = 0;
	int	 num;

	/* wrap up the postscript output */
	if (genps_end() != 0)
	    return -1;		/* error, return now */

	/* add a showpage so ghostscript will produce output */
	fprintf(tfp, "showpage\n");

	if (pclose(tfp) != 0) {
	    fprintf(stderr,"Error in ghostcript command\n");
	    fprintf(stderr,"command was: %s\n", gscom);
	    return -1;
	}
	(void) signal(SIGPIPE, SIG_DFL);

	/* all ok so far */

	return status;
}

struct driver dev_pdf = {
  	genpdf_option,
	genpdf_start,
	genps_arc,
	genps_ellipse,
	genps_line,
	genps_spline,
	genps_text,
	genpdf_end,
	INCLUDE_TEXT
};


