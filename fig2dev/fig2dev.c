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
 *	Fig2dev : General Fig code translation program
 *
*/
#if defined(hpux) || defined(SYSV) || defined(SVR4)
#include <sys/types.h>
#endif
#include <sys/file.h>
#include <stdio.h>
#include "patchlevel.h"
#include "fig2dev.h"
#include "object.h"
#include "drivers.h"

extern int fig_getopt();
extern char *optarg;
extern int optind;
char	lang[40];

struct driver *dev = NULL;

#ifdef I18N
char		Usage[] = "Usage: %s [-L language] [-f font] [-s size] [-m scale] [-j] [input [output]]\n";
Boolean support_i18n = FALSE;
#else
char		Usage[] = "Usage: %s [-L language] [-f font] [-s size] [-m scale] [input [output]]\n";
#endif  /* I18N */
char		Err_badarg[] = "Argument -%c unknown to %s driver.";
char		Err_incomp[] = "Incomplete %s object at line %d.";
char		Err_mem[] = "Running out of memory.";

char		*prog;
char		*from = NULL, *to = NULL;
char		*name = NULL;
int		font_size = 0;
double		mag = 1.0;
FILE		*tfp = NULL;
int		llx = 0, lly = 0, urx = 0, ury = 0;
Boolean		landscape;
Boolean		center;
Boolean		orientspec=FALSE;	/* set if the user specifies the orientation */
Boolean		centerspec=FALSE;	/* set if the user specifies the justification */
Boolean		magspec=FALSE;		/* set if the user specifies the magnification */
Boolean		multispec=FALSE;	/* set if the user specifies multiple pages */
Boolean		paperspec=FALSE;	/* set if the user specifies the paper size */
Boolean		pats_used, pattern_used[NUMPATTERNS];
Boolean		multi_page = FALSE;	/* multiple page option for PostScript */
Boolean		metric;			/* true if file specifies Metric */
char		papersize[20];		/* paper size */
float		THICK_SCALE;		/* convert line thickness from screen res. */
					/* calculated in read_objects() */
char		lang[40];		/* selected output language */

struct obj_rec {
	void (*gendev)();
	char *obj;
	int depth;
};

/* be sure to update NUMPAPERSIZES in fig2dev.h if this table changes */

struct paperdef paperdef[] =
{
    {"Letter", 612, 792}, 	/* 8.5" x 11" */
    {"Legal", 612, 1008}, 	/* 8.5" x 14" */
    {"Tabloid", 792, 1224}, 	/*  11" x 17" */
    {"A", 612, 792},		/* 8.5" x 11" (letter) */
    {"B", 792, 1224},		/*  11" x 17" (tabloid) */
    {"C", 1224, 1584},		/*  17" x 22" */
    {"D", 1584, 2448},		/*  22" x 34" */
    {"E", 2448, 3168},		/*  34" x 44" */
    {"B5", 516, 729}, 		/* 18.2cm x  25.7cm */
    {"A4", 595, 842}, 		/* 21  cm x  29.7cm */
    {"A3", 842, 1190},		/* 29.7cm x  42  cm */
    {"A2", 1190, 1684},		/* 42  cm x  59.4cm */
    {"A1", 1684, 2383},		/* 59.4cm x  84.1cm */
    {"A0", 2383, 3370},		/* 84.1cm x 118.9cm */ 
    {NULL, 0, 0}
};


put_msg(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)
char   *format, *arg1, *arg2, *arg3, *arg4, *arg5, *arg6, *arg7, *arg8;
{
	fprintf(stderr, format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
	fprintf(stderr, "\n");
	}

get_args(argc, argv)
int	 argc;
char	*argv[];
{
  	int	c, i;
	double	atof();

	prog = *argv;
/* add :? */
	/* sum of all arguments */
#ifdef I18N
	while ((c = fig_getopt(argc, argv, "aAcC:d:ef:hl:L:Mm:n:q:Pp:s:S:t:vVx:X:y:Y:wWz:j?")) != EOF) {
#else
	while ((c = fig_getopt(argc, argv, "aAcC:d:ef:hl:L:Mm:n:q:Pp:s:S:t:vVx:X:y:Y:wWz:?")) != EOF) {
#endif

	  /* generic option handling */
	  switch (c) {

		case 'h':	/* print version message for -h too */
		case 'V': 
			fprintf(stderr, "fig2dev Version %s Patchlevel %s\n",
							VERSION, PATCHLEVEL);
			if (c == 'h')
			    help_msg();
			exit(0);
			break;

		case 'L':			/* set output language */
		    /* save language for gen{gif,jpg,pcx,xbm,xpm,ppm,tif} */
		    strncpy(lang,optarg,sizeof(lang)-1);
		    for (i=0; *drivers[i].name; i++) 
			if (!strcmp(lang, drivers[i].name))
				dev = drivers[i].dev;
		    if (!dev) {
			fprintf(stderr,
				"Unknown graphics language %s\n", lang);
			fprintf(stderr,"Known languages are:\n");
			/* display available languages - 23/01/90 */
			for (i=0; *drivers[i].name; i++)
				fprintf(stderr,"%s ",drivers[i].name);
			fprintf(stderr,"\n");
			exit(1);
		    }
		    break;

		case 's':			/* set default font size */
		    font_size = atoi(optarg);
		    /* max size is checked in respective drivers */
		    if (font_size <= 0)
			font_size = DEFAULT_FONT_SIZE;
		    font_size = round(font_size * mag);
		    break;

		case 'm':			/* set magnification */
		    mag = atof(optarg);
		    magspec = TRUE;		/* user-specified */
		    break;

#ifdef I18N
		case 'j':			/* set magnification */
		    support_i18n = TRUE;
		    continue;  /* don't pass this option to driver */
#endif /* I18N */
		case '?':			/* usage 		*/
			fprintf(stderr,Usage,prog);
			exit(1);
	    }

	    /* pass options through to driver */
	    if (!dev) {
		fprintf(stderr, "No graphics language specified.\n");
		exit(1);
	    }
	    dev->option(c, optarg);
      	}
      	if (!dev) {
		fprintf(stderr, "No graphics language specified.\n");
		exit(1);
      	}

	if (optind < argc)
		from = argv[optind++];  /*  from file  */
	if (optind < argc)
		to   = argv[optind];  /*  to file    */
}

main(argc, argv)
int	 argc;
char	*argv[];
{
	F_compound	objects;
	int		status;

	get_args(argc, argv);

	if (from)
	    status = read_fig(from, &objects);
	else	/* read from stdin */
	    status = readfp_fig(stdin, &objects);

	if (status != 0) {
	    if (from) read_fail_message(from, status);
	    exit(1);
	    }

	if (to == NULL)
	    tfp = stdout;
	else if ((tfp = fopen(to, "w")) == NULL) {
	    fprintf(stderr, "Couldn't open %s", to);
	    fprintf(stderr, Usage, prog);
	    exit(1);
	}

	/* if metric, adjust scale for difference between 
	   FIG PIX/CM (450) and actual (472.44) */
	if (metric)
		mag *= 80.0/76.2;

	gendev_objects(&objects, dev);
	if ((tfp != stdout) && (tfp != 0)) 
	    (void)fclose(tfp);
	exit(0);
}

help_msg()
{
	fprintf(stderr,"General Options:\n");
	fprintf(stderr,"  -L language	choose output language (this must be first)\n");
	fprintf(stderr,"  -m mag	set magnification\n");
	fprintf(stderr,"  -f font	set default font\n");
	fprintf(stderr,"  -s size	set default font size in points\n");
	fprintf(stderr,"  -h		print this message, fig2dev version number and exit\n");
	fprintf(stderr,"  -V		print fig2dev version number and exit\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"PostScript Options:\n");
	fprintf(stderr,"  -c		center figure on page\n");
	fprintf(stderr,"  -e		put figure at edge of page\n");
	fprintf(stderr,"  -l dummyarg	landscape mode\n");
	fprintf(stderr,"  -p dummyarg	portrait mode\n");
	fprintf(stderr,"  -M		generate multiple pages for large figure\n");
	fprintf(stderr,"  -P		generate \"showpage\" command for printing\n");
	fprintf(stderr,"  -n name	set title part of PostScript output to name\n");
	fprintf(stderr,"  -x offset	shift figure left/right by offset units (1/72 inch)\n");
	fprintf(stderr,"  -y offset	shift figure up/down by offset units (1/72 inch)\n");
	fprintf(stderr,"  -z papersize	set the papersize (see man pages for available sizes)\n");
	fprintf(stderr,"LaTeX Options:\n");
	fprintf(stderr,"  -l lwidth	set threshold between thin and thick lines to lwidth\n");
	fprintf(stderr,"  -d dmag	set separate magnification for length of line dashes to dmag\n");
	fprintf(stderr,"  -v		verbose mode\n");
	fprintf(stderr,"PSTEX Options:\n");
	fprintf(stderr,"  -n name	set title part of PostScript output to name\n");
	fprintf(stderr,"  -p name	name of the PostScript file to be overlaid\n");
	fprintf(stderr,"EPIC Options:\n");
	fprintf(stderr,"  -A scale	scale arrowheads by dividing their size by scale\n");	
	fprintf(stderr,"  -l lwidth	use \"thicklines\" when width of line is > lwidth\n");
	fprintf(stderr,"  -v		include comments in the output\n");
	fprintf(stderr,"  -P		generate a complete LaTeX file\n");
	fprintf(stderr,"  -S scale	scale figure\n");
	fprintf(stderr,"  -W		enable variable line width\n");
	fprintf(stderr,"  -w		disable variable line width\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"TK Options:\n");
	fprintf(stderr,"  -l dummyarg	landscape mode\n");
	fprintf(stderr,"  -p dummyarg	portrait mode\n");
	fprintf(stderr,"  -P		generate canvas of full page size instead of figure bounds\n");
	fprintf(stderr,"  -z papersize	set the papersize (see man pages for available sizes)\n");
	fprintf(stderr,"PIC Options:\n");
	fprintf(stderr,"  -p ext	enables certain PIC extensions (see man pages)\n");
	fprintf(stderr,"METAFONT Options:\n");
	fprintf(stderr,"  -C code	specifies the starting METAFONT font code\n");
	fprintf(stderr,"  -n name	name to use in the output file\n");
	fprintf(stderr,"  -p pen_mag	linewidth magnification compared to the original figure\n");
	fprintf(stderr,"  -t top	specifies the top of the coordinate system\n");
	fprintf(stderr,"  -x xneg	specifies minimum x coordinate of figure (inches)\n");
	fprintf(stderr,"  -y yneg	specifies minimum y coordinate of figure (inches)\n");
	fprintf(stderr,"  -X xpos	specifies maximum x coordinate of figure (inches)\n");
	fprintf(stderr,"  -Y xpos	specifies maximum y coordinate of figure (inches)\n");
	fprintf(stderr,"JPEG Options:\n");
	fprintf(stderr,"  -q quality	specify image quality factor (0-100)\n");
	fprintf(stderr,"GIF Options:\n");
	fprintf(stderr,"  -t color	specify GIF transparent color in hexadecimal (e.g. #ff0000=red)\n");
	fprintf(stderr,"TEXTYL Options: None\n");
	fprintf(stderr,"TPIC Options: None\n");
	fprintf(stderr,"IBM-GL Options:\n");
	fprintf(stderr,"  -a		select ISO A4 paper size if default is ANSI A, or vice versa\n");
	fprintf(stderr,"  -c		generate instructions for IBM 6180 plotter\n");
	fprintf(stderr,"  -d xll,yll,xur,yur	restrict plotting to area specified by coords\n");
	fprintf(stderr,"  -f fontfile	load text character specs from table in file\n");
	fprintf(stderr,"  -l pattfile	load patterns for pattern fill from file\n");
	fprintf(stderr,"  -m mag,x0,y0	magnification with optional offset in inches\n");
	fprintf(stderr,"  -p pensfile	load plotter pen specs from file\n");
	fprintf(stderr,"  -P		rotate figure to portrait (default is landscape)\n");
	fprintf(stderr,"  -S speed	set pen speed in cm/sec\n");
	fprintf(stderr,"  -v		print figure upside-down in portrait or backwards in landscape\n");
}

/* count primitive objects & create pointer array */
static int compound_dump(com, array, count, dev)
F_compound *com;
struct obj_rec *array;
int count;
struct driver *dev;
{
  	F_arc		*a;
	F_compound	*c;
	F_ellipse	*e;
	F_line		*l;
	F_spline	*s;
	F_text		*t;

	for (c = com->compounds; c != NULL; c = c->next)
	  count = compound_dump(c, array, count, dev);
	for (a = com->arcs; a != NULL; a = a->next) {
	  if (array) {
		array[count].gendev = dev->arc;
		array[count].obj = (char *)a;
		array[count].depth = a->depth;
	  }
	  count += 1;
	}
	for (e = com->ellipses; e != NULL; e = e->next) {
	  if (array) {
		array[count].gendev = dev->ellipse;
		array[count].obj = (char *)e;
		array[count].depth = e->depth;
	  }
	  count += 1;
	}
	for (l = com->lines; l != NULL; l = l->next) {
	  if (array) {
		array[count].gendev = dev->line;
		array[count].obj = (char *)l;
		array[count].depth = l->depth;
	  }
	  count += 1;
	}
	for (s = com->splines; s != NULL; s = s->next) {
	  if (array) {
		array[count].gendev = dev->spline;
		array[count].obj = (char *)s;
		array[count].depth = s->depth;
	  }
	  count += 1;
	}
	for (t = com->texts; t != NULL; t = t->next) {
	  if (array) {
		array[count].gendev = dev->text;
		array[count].obj = (char *)t;
		array[count].depth = t->depth;
	  }
	  count += 1;
	}
	return count;
}

gendev_objects(objects, dev)
F_compound	*objects;
struct driver *dev;
{
	int obj_count, rec_comp();
	struct obj_rec *rec_array, *r; 

	if (0 == (double)objects->nwcorner.x) {
	    fprintf(stderr, "Resolution is zero!! default to 80 ppi\n");
	    objects->nwcorner.x = 80;
	    }
	if (objects->nwcorner.y != 1 && objects->nwcorner.y != 2) {
	    fprintf(stderr, "Wrong coordinate system; cannot continue\n");
	    return;
	    }

	/* Compute bounding box of objects, supressing texts if indicated */
	compound_bound(objects, &llx, &lly, &urx, &ury, dev->text_include);

	/* dump object pointers to an array */
	obj_count = compound_dump(objects, 0, 0, dev);
	if (!obj_count) {
	    fprintf(stderr, "No object\n");
	    return;
	    }
	rec_array = (struct obj_rec *)malloc(obj_count*sizeof(struct obj_rec));
	(void)compound_dump(objects, rec_array, 0, dev);

	/* sort object array by depth */
	qsort(rec_array, obj_count, sizeof(struct obj_rec), rec_comp);

	/* generate header */
	(*dev->start)(objects);

	/* generate objects in sorted order */
	for (r = rec_array; r<rec_array+obj_count; r++)
		(*(r->gendev))(r->obj);

	/* generate trailer */
	(*dev->end)();
	free(rec_array);
}

int rec_comp(r1, r2)
struct obj_rec *r1, *r2;
{
	return (r2->depth - r1->depth);
}

/* null operation */
void gendev_null() {;}
