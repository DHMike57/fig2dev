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
 *		calls ps2pdf (which uses ghostscript) to convert it to pdf.
 */

#if defined(hpux) || defined(SYSV) || defined(SVR4)
#include <sys/types.h>
#endif
#include <sys/file.h>
#include "fig2dev.h"
#include "genps.h"
#include "object.h"
#include "texfonts.h"

extern void 
	genps_arc(),
	genps_ellipse(),
	genps_line(),
	genps_spline(),
	genps_text();

static char com[1000];
Boolean	direct;
FILE	*saveofile;
char	*ofile;
char	 tempfile[PATH_MAX];

void
genpdf_option(opt, optarg)
char opt;
char *optarg;
{
	/* just use the ps options */
	genps_option(opt, optarg);
}

void
genpdf_start(objects)
F_compound	*objects;
{

	/* change output from ps driver to a temp file because
	   ps2pdf only works with files, not pipes */
	/* but first close the output file that main() opened */
	saveofile = tfp;
	if (tfp != stdout)
	    fclose(tfp);
	sprintf(tempfile, "fig2dev-%d", getpid());
	if ((tfp = fopen(tempfile,"w" )) == 0) {
	    fprintf(stderr,"Can't open temporary file %s\n",tempfile);
	    exit(1);
	}
	genps_start(objects);
}

int
genpdf_end()
{
	char	com[500],tempofile[PATH_MAX];
	int	status = 0;

	sprintf(tempfile, "fig2dev-%d", getpid());

	/* wrap up the postscript output */
	if (genps_end() != 0)
	    return -1;

	/* add a showpage so the pdf will have a page */
	fprintf(tfp, "showpage\n");
	fclose(tfp);

	/* if no output file specified, put in temp and cat it */
	if (to==NULL) {
	    sprintf(tempofile,"fig2dev-%d.pdf", getpid());
	    to = tempofile;
	}
	/* make up the command for ps2pdf */
	sprintf(com, "ps2pdf %s %s", tempfile, to);
	/* run it */
	if (system(com) != 0)
	    status = -1;

	/* finally, remove the temporary file */
	unlink(tempfile);
	/* cat the temp output file if any */
	if (to == tempofile) {
	    sprintf(com,"cat %s",tempofile);
	    system(com);
	    /* and remove it */
	    unlink(tempofile);
	}
	/* we've already closed the original output file */
	tfp = 0;

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


