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
 *	genbitmaps.c : bitmap driver for fig2dev 
 *
 *	Author: Brian V. Smith
 *		Handles AutoCad Slide, GIF, JPEG, TIFF, PCX, PNG, XBM and XPM.
 *		Uses genps functions to generate PostScript output then
 *		calls ghostscript to convert it to the output language
 *		if ghostscript has a driver for that language, or to ppm
 *		if otherwise. If the latter, ppmtoxxx is then called to make
 *		the final XXX file.
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

extern void 
	genps_arc(),
	genps_ellipse(),
	genps_line(),
	genps_spline(),
	genps_text();

static char gscom[1000],*gsdev,tmpname[PATH_MAX];
Boolean	direct;
FILE	*saveofile;
char	*ofile;
int	width,height;
int	jpeg_quality=75;
static	int	border_margin = 0;
static	int	smooth = 1;
static	int	debug = 0;

void
gs_broken_pipe(int sig)
{
  fprintf(stderr,"fig2dev: broken pipe (GhostScript aborted?)\n");
  fprintf(stderr,"command was: %s\n", gscom);
  exit(1);
}

void
genbitmaps_option(opt, optarg)
char opt;
char *optarg;
{
    switch (opt) {

	case 'b':			/* border margin around bitmap */
	    sscanf(optarg,"%d",&border_margin);
	    break;

	case 'g':			/* background color (handled in postscript gen) */
	    if (lookup_db_color(optarg,&background) >= 0) {
		bgspec = True;
	    } else {
		fprintf(stderr,"Can't parse color '%s', ignoring background option\n",
				optarg);
	    }
	    break;

	case 'q':			/* jpeg image quality */
	    if (strcmp(lang,"jpeg") != 0)
		fprintf(stderr,"-q option only allowed for jpeg quality; ignored\n");
	    sscanf(optarg,"%d",&jpeg_quality);
	    break;

	case 't':			/* GIF transparent color */
	    if (strcmp(lang,"gif") != 0)
		fprintf(stderr,"-t option only allowed for GIF transparent color; ignored\n");
	    (void) strcpy(gif_transparent,optarg);
	    transspec = True;
	    break;

	case 'S':			/* smoothing factor */
	    sscanf(optarg,"%d",&smooth);
	    if (smooth < 1) {
		fprintf(stderr,"fig2dev: bad value for -S option: %s\n", optarg);
		exit(1);
	    }
	    break;

	case 'f':			/* ignore magnification, font sizes and lang here */
	case 'm':
	case 's':
	case 'L':
	    break;

	default:
	    put_msg(Err_badarg, opt, lang);
	    break;
    }
}

void
genbitmaps_start(objects)
F_compound	*objects;
{
    char extra_options[200];
    float bd;

    bd = border_margin * THICK_SCALE;

    llx -= bd;
    lly -= bd;
    urx += bd;
    ury += bd;

    /* make command for ghostscript */
    width=round(mag*smooth*(urx-llx)/THICK_SCALE);
    height=round(mag*smooth*(ury-lly)/THICK_SCALE);

    /* Add conditionals here if gs has a driver built-in */
    /* gs has a driver for png, ppm, pcx, jpeg and tiff */

    direct = True;
    ofile = to;
    extra_options[0]='\0';

    gsdev = NULL;
    /* if we're smoothing, we'll generate magnified ppm then convert it later */
    if (smooth == 1) {
	if (strcmp(lang,"png")==0) {
	    gsdev="png16m";
	} else if (strcmp(lang,"pcx")==0) {
	    gsdev="pcx256";
	} else if (strcmp(lang,"ppm")==0) {
	    gsdev="ppmraw";
	} else if (strcmp(lang,"tiff")==0) {
	    /* use the 24-bit - unfortunately, it doesn't use compression */
	    gsdev="tiff24nc";
	} else if (strcmp(lang,"jpeg")==0) {
	    gsdev="jpeg";
	    /* set quality for JPEG */
	    sprintf(extra_options,"-dJPEGQ=%d",jpeg_quality);
	}
    }
    /* no driver in gs or we're smoothing, use ppm output then use ppmtoxxx later */
    if (gsdev == NULL) {
	gsdev="ppmraw";
	/* make a unique name for the temporary ppm file */
	sprintf(tmpname,"%s/f2d%d.ppm",TMPDIR,getpid());
	ofile = tmpname;
	direct = False;
    }
    /* make up the command for gs */
    sprintf(gscom, "gs -q -dSAFER -sDEVICE=%s -r%d -g%dx%d -sOutputFile=%s %s -",
		   gsdev, 80 * smooth, width, height, ofile, extra_options);
    /* divert output from ps driver to the pipe into ghostscript */
    /* but first close the output file that main() opened */
    saveofile = tfp;
    if (tfp != stdout)
	fclose(tfp);

    (void) signal(SIGPIPE, gs_broken_pipe);
    if ((tfp = popen(gscom,"w" )) == 0) {
	fprintf(stderr,"fig2dev: Can't open pipe to ghostscript\n");
	fprintf(stderr,"command was: %s\n", gscom);
	exit(1);
    }
    /* generate eps and not ps */
    epsflag = True;
    genps_start(objects);
}

int
genbitmaps_end()
{
	char	 com[PATH_MAX+200],com1[200];
	char	 errfname[PATH_MAX];
	char	*tmpname1;
	int	 status;

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
	status = 0;

	/* for the formats that are only 8-bits, reduce the colors to 256 */
	/* and pipe through the ppm converter for that format */
	if (!direct) {
	    tmpname1 = tmpname;
	    if (smooth == 1) {
		strcpy(com, "(");
	    } else {
		sprintf(com, "(pnmscale %f %s | ", 1.0 / smooth, tmpname);
		tmpname1 = "";
	    }
	    if (strcmp(lang, "gif")==0) {
		if (gif_transparent[0]) {
		    /* escape the first char of the transparent color (#) for the shell */
		    sprintf(com1,"ppmquant 256 %s | ppmtogif -transparent \\%s",
			tmpname1, gif_transparent);
		} else {
		    sprintf(com1,"ppmquant 256 %s | ppmtogif",tmpname1);
		}
	    } else if (strcmp(lang, "jpeg")==0) {
		sprintf(com1, "cjpeg -quality %d %s", jpeg_quality, tmpname1);
	    } else if (strcmp(lang, "xbm")==0) {
		sprintf(com1,"ppmtopgm %s | pgmtopbm | pbmtoxbm",tmpname1);
	    } else if (strcmp(lang, "xpm")==0) {
		sprintf(com1,"ppmquant 256 %s | ppmtoxpm",tmpname1);
	    } else if (strcmp(lang, "sld")==0) {
		sprintf(com1,"ppmtoacad %s",tmpname1);
	    } else if (strcmp(lang, "pcx")==0) {
		sprintf(com1, "ppmtopcx %s", tmpname1);
	    } else if (strcmp(lang, "png")==0) {
		sprintf(com1, "pnmtopng %s", tmpname1);
	    } else if (strcmp(lang, "ppm")==0) {
		sprintf(com1, "cat %s", tmpname1);
	    } else if (strcmp(lang, "tiff")==0) {
		sprintf(com1, "pnmtotiff %s", tmpname1);
	    } else {
		fprintf(stderr, "fig2dev: unsupported image format: %s\n", lang);
		exit(1);
	    }
	    strcat(com, com1);

	    if (saveofile != stdout) {
		/* finally, route output from ppmtoxxx to final output file, if
		   not going to stdout */
		strcat(com," > ");
		strcat(com,to);
	    }
	    /* close off parenthesized command stream */
	    strcat(com,")");

	    /* make a unique name for an error file */
	    sprintf(errfname,"%s/f2d%d.err",TMPDIR,getpid());

	    if (debug)
		fprintf(stderr,"Calling: %s\n",com);
	    /* send all messages to error file */
	    strcat(com," 2> ");
	    strcat(com,errfname);

	    /* execute the ppm program */
	    if ((status=system(com)) != 0) {
		FILE *errfile;

		/* force to -1 */
		status = -1;

		/* seems to be a race condition where not all of the messages
		   make it into the error file before we open it, so we'll wait a tad */
		sleep(1);
		errfile = fopen(errfname,"r");
		fprintf(stderr,"fig2dev: error while converting image.\n");
		fprintf(stderr,"Command used:\n  %s\n",com);
		fprintf(stderr,"Messages resulting:\n");
		if (errfile == 0)
		    fprintf(stderr,"can't opern error file %s\n",errfname);
		else {
		    while (!feof(errfile)) {
			if (fgets(com, sizeof(com)-1, errfile) == NULL)
			    break;
			fprintf(stderr,"  %s",com);
		    }
		    fclose(errfile);
		}
	    }

	    /* finally, remove the temporary file and the error file */
	    unlink(tmpname);
	    unlink(errfname);
	}
	/* we've already closed the original output file */
	tfp = 0;

	return status;
}

struct driver dev_bitmaps = {
  	genbitmaps_option,
	genbitmaps_start,
	genps_arc,
	genps_ellipse,
	genps_line,
	genps_spline,
	genps_text,
	genbitmaps_end,
	INCLUDE_TEXT
};


