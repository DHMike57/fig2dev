/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1991 by Micah Beck
 * Parts Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-1999 by Brian V. Smith
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
 *	Fig2dev : General Fig code translation program
 *
*/
#if defined(hpux) || defined(SYSV) || defined(SVR4)
#include <sys/types.h>
#endif
#include <sys/file.h>
#include "fig2dev.h"
#include "alloc.h"
#include "object.h"
#include "drivers.h"

extern	int	 fig_getopt();
extern	char	*optarg;
extern	int	 optind;
char		 lang[40];

/* hex names for Fig colors */
char	*Fig_color_names[] = {
		"#000000", "#0000ff", "#00ff00", "#00ffff",
		"#ff0000", "#ff00ff", "#ffff00", "#ffffff",
		"#000090", "#0000b0", "#0000d0", "#87ceff", 
		"#009000", "#00b000", "#00d000", "#009090",
		"#00b0b0", "#00d0d0", "#900000", "#b00000",
		"#d00000", "#900090", "#b000b0", "#d000d0",
		"#803000", "#a04000", "#c06000", "#ff8080",
		"#ffa0a0", "#ffc0c0", "#ffe0e0", "#ffd700"
		};

struct driver *dev = NULL;

#ifdef I18N
char		Usage[] = "Usage: %s [-L language] [-f font] [-s size] [-m scale] [-j] [input [output]]\n";
Boolean support_i18n = False;
#else
char		Usage[] = "Usage: %s [-L language] [-f font] [-s size] [-m scale] [input [output]]\n";
#endif  /* I18N */
char		Err_badarg[] = "Argument -%c unknown to %s driver.";
char		Err_mem[] = "Running out of memory.";

char		*prog;
char		*from = NULL, *to = NULL;
char		*name = NULL;
int		font_size = 0;
double		mag = 1.0;
FILE		*tfp = NULL;

double		ppi;			/* Fig file resolution (e.g. 1200) */
int		llx = 0, lly = 0, urx = 0, ury = 0;
Boolean		landscape;
Boolean		center;
Boolean		orientspec = False;	/* set if the user specs. the orientation */
Boolean		centerspec = False;	/* set if the user specs. the justification */
Boolean		magspec = False;	/* set if the user specs. the magnification */
Boolean		transspec = False;	/* set if the user specs. the GIF transparent color */
Boolean		multispec = False;	/* set if the user specs. multiple pages */
Boolean		paperspec = False;	/* set if the user specs. the paper size */
Boolean		pats_used, pattern_used[NUMPATTERNS];
Boolean		multi_page = False;	/* multiple page option for PostScript */
Boolean		metric;			/* true if file specifies Metric */
char		gif_transparent[20]="\0"; /* GIF transp color hex name (e.g. #ff00dd) */
char		papersize[20];		/* paper size */
float		THICK_SCALE;		/* convert line thickness from screen res. */
					/* calculated in read_objects() */
char		lang[40];		/* selected output language */
RGB		background;		/* background (if specified by -g) */
Boolean		bgspec = False;		/* flag to say -g was specified */
char		gscom[1000];		/* to build up a command for ghostscript */

Boolean	psencode_header_done = False; /* if we have already emitted PSencode header */
Boolean	transp_header_done = False;   /* if we have already emitted transparent image header */

struct obj_rec {
	void (*gendev)();
	char *obj;
	int depth;
};

#define NUMDEPTHS 100

struct depth_opts {
        int d1,d2;
} depth_opt[NUMDEPTHS];
int	depth_index = 0;
char	depth_op;		/* '+' for skip all but those listed */

/* be sure to update NUMPAPERSIZES in fig2dev.h if this table changes */

struct paperdef paperdef[] =
{
    {"Letter", 612, 792}, 	/*  8.5" x 11" */
    {"Legal", 612, 1008}, 	/*  8.5" x 14" */
    {"Tabloid", 792, 1224}, 	/*   11" x 17" */
    {"A",   612, 792},		/*  8.5" x 11" (letter) */
    {"B",   792, 1224},		/*   11" x 17" (tabloid) */
    {"C",  1224, 1584},		/*   17" x 22" */
    {"D",  1584, 2448},		/*   22" x 34" */
    {"E",  2448, 3168},		/*   34" x 44" */
    {"A9",  105, 148},		/*   37 mm x   52 mm */
    {"A8",  148, 210},		/*   52 mm x   74 mm */
    {"A7",  210, 297},		/*   74 mm x  105 mm */
    {"A6",  297, 420},		/*  105 mm x  148 mm */
    {"A5",  420, 595},		/*  148 mm x  210 mm */
    {"A4",  595, 842}, 		/*  210 mm x  297 mm */
    {"A3",  842, 1190},		/*  297 mm x  420 mm */
    {"A2", 1190, 1684},		/*  420 mm x  594 mm */
    {"A1", 1684, 2383},		/*  594 mm x  841 mm */
    {"A0", 2383, 3370},		/*  841 mm x 1189 mm */ 
    {"B10",  91,  127},		/*   32 mm x   45 mm */
    {"B9",  127,  181},		/*   45 mm x   64 mm */
    {"B8",  181,  258},		/*   64 mm x   91 mm */
    {"B7",  258,  363},		/*   91 mm x  128 mm */
    {"B6",  363,  516},		/*  128 mm x  182 mm */
    {"B5",  516,  729}, 	/*  182 mm x  257 mm */
    {"B4",  729, 1032},		/*  257 mm x  364 mm */
    {"B3", 1032, 1460},		/*  364 mm x  515 mm */
    {"B2", 1460, 2064},		/*  515 mm x  728 mm */
    {"B1", 2064, 2920},		/*  728 mm x 1030 mm */
    {"B0", 2920, 4127},		/* 1030 mm x 1456 mm */
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
  	int	 c, i;
	double	 atof();

	prog = *argv;
/* add :? */
	/* sum of all arguments */
#ifdef I18N
	while ((c = fig_getopt(argc, argv, "aAb:cC:d:D:ef:g:hkl:L:Mm:n:q:Pp:rs:S:t:vVx:X:y:Y:wWz:j?")) != EOF) {
#else
	while ((c = fig_getopt(argc, argv, "aAb:cC:d:D:ef:g:hkl:L:Mm:n:q:Pp:rs:S:t:vVx:X:y:Y:wWz:?")) != EOF) {
#endif

	  /* generic option handling */
	  switch (c) {

		case 'h':	/* print version message for -h too */
		case 'V': 
		    printf("fig2dev Version %s Patchlevel %s\n",
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
		    magspec = True;		/* user-specified */
		    break;

#ifdef I18N
		case 'j':			/* set magnification */
		    support_i18n = True;
		    continue;  /* don't pass this option to driver */
#endif /* I18N */
	        case 'D':	                /* depth filtering */
		    depth_option(optarg);
		    continue;	/* this opts parser is a mess */
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
		from = argv[optind++];	/*  from file  */
	if (optind < argc)
		to   = argv[optind];	/*  to file    */
}

main(argc, argv)
int	 argc;
char	*argv[];
{
	F_compound	objects;
	int		status;

#ifdef HAVE_SETMODE
	setmode(1,O_BINARY); /* stdout is binary */
#endif

	get_args(argc, argv);

	if (from)
	    status = read_fig(from, &objects);
	else	/* read from stdin */
	    status = readfp_fig(stdin, &objects);

	if (status != 0) {
	    if (from) 
		read_fail_message(from, status);
	    exit(1);
	}

	if (to == NULL)
	    tfp = stdout;
	else if ((tfp = fopen(to, "wb")) == NULL) {
	    fprintf(stderr, "Couldn't open %s\n", to);
	    exit(1);
	}

	/* if metric, adjust scale for difference between 
	   FIG PIX/CM (450) and actual (472.44) */
	if (metric)
		mag *= 80.0/76.2;

	status = gendev_objects(&objects, dev);
	if ((tfp != stdout) && (tfp != 0)) 
	    (void)fclose(tfp);
	exit(status);
}

help_msg()
{
    int i;

    printf("General Options:\n");
    printf("  -L language	choose output language (this must be first)\n");
    /* display available languages - 23/01/90 */
    printf("                Available languages are:");
    for (i=0; *drivers[i].name; i++) {
	if (i%9 == 0)
	printf("\n\t\t  ");
	printf("%s ",drivers[i].name);
    }
    printf("\n");
    printf("  -m mag	set magnification\n");
    printf("  -D +/-list	include or exclude depths listed\n");
    printf("  -f font	set default font\n");
    printf("  -s size	set default font size in points\n");
    printf("  -h		print this message, fig2dev version number and exit\n");
    printf("  -V		print fig2dev version number and exit\n");
#ifdef I18N
    printf("  -j		enable i18n facility\n");
#endif
    printf("\n");

    printf("CGM Options:\n");
    printf("  -r		Position arrowheads for CGM viewers that display rounded arrowheads\n");

    printf("EPIC Options:\n");
    printf("  -A scale	scale arrowheads by dividing their size by scale\n");	
    printf("  -l lwidth	use \"thicklines\" when width of line is > lwidth\n");
    printf("  -v		include comments in the output\n");
    printf("  -P		generate a complete LaTeX file\n");
    printf("  -S scale	scale figure\n");
    printf("  -W		enable variable line width\n");
    printf("  -w		disable variable line width\n");

    printf("EPS (Encapsulated PostScript) Options:\n");
    printf("  -g color	background color\n");
    printf("  -n name	set title part of PostScript output to name\n");

    printf("IBM-GL Options:\n");
    printf("  -a		select ISO A4 paper size if default is ANSI A, or vice versa\n");
    printf("  -c		generate instructions for IBM 6180 plotter\n");
    printf("  -d xll,yll,xur,yur	restrict plotting to area specified by coords\n");
    printf("  -f fontfile	load text character specs from table in file\n");
    printf("  -l pattfile	load patterns for pattern fill from file\n");
    printf("  -m mag,x0,y0	magnification with optional offset in inches\n");
    printf("  -p pensfile	load plotter pen specs from file\n");
    printf("  -P		rotate figure to portrait (default is landscape)\n");
    printf("  -S speed	set pen speed in cm/sec\n");
    printf("  -v		print figure upside-down in portrait or backwards in landscape\n");

    printf("GIF Options:\n");
    printf("  -b width	specify width of blank border around figure\n");
    printf("  -g color	background color\n");
    printf("  -S smooth	specify smoothing factor [2-3 reasonable]\n");
    printf("  -t color	specify GIF transparent color in hexadecimal (e.g. #ff0000=red)\n");

#ifdef USE_JPEG
    printf("JPEG Options:\n");
    printf("  -b width	specify width of blank border around figure\n");
    printf("  -g color	background color\n");
    printf("  -q quality	specify image quality factor (0-100)\n");
    printf("  -S smooth	specify smoothing factor [2-3 reasonable]\n");
#endif /* USE_JPEG */

    printf("Options for all other bitmap languages:\n");
    printf("  -b width	specify width of blank border around figure\n");
    printf("  -g color	background color\n");
    printf("  -S smooth	specify smoothing factor [2-3 reasonable]\n");

    printf("LaTeX Options:\n");
    printf("  -d dmag	set separate magnification for length of line dashes to dmag\n");
    printf("  -l lwidth	set threshold between thin and thick lines to lwidth\n");
    printf("  -v		verbose mode\n");

    printf("MAP (HTML image map) Options:\n");
    printf("  -b width	specify width of blank border around figure\n");

    printf("METAFONT Options:\n");
    printf("  -C code	specifies the starting METAFONT font code\n");
    printf("  -n name	name to use in the output file\n");
    printf("  -p pen_mag	linewidth magnification compared to the original figure\n");
    printf("  -t top	specifies the top of the coordinate system\n");
    printf("  -x xneg	specifies minimum x coordinate of figure (inches)\n");
    printf("  -y yneg	specifies minimum y coordinate of figure (inches)\n");
    printf("  -X xpos	specifies maximum x coordinate of figure (inches)\n");
    printf("  -Y xpos	specifies maximum y coordinate of figure (inches)\n");

    printf("PIC Options:\n");
    printf("  -p ext	enables certain PIC extensions (see man pages)\n");

    printf("PostScript and PDF Options:\n");
    printf("  -b width	specify width of blank border around figure\n");
    printf("  -c		center figure on page\n");
    printf("  -e		put figure at left edge of page\n");
    printf("  -g color	background color\n");
    printf("  -l dummyarg	landscape mode\n");
    printf("  -p dummyarg	portrait mode\n");
    printf("  -M		generate multiple pages for large figure\n");
    printf("  -n name	set title part of PostScript output to name\n");
    printf("  -x offset	shift figure left/right by offset units (1/72 inch)\n");
    printf("  -y offset	shift figure up/down by offset units (1/72 inch)\n");
    printf("  -z papersize	set the papersize (see man pages for available sizes)\n");

    printf("PSTEX Options:\n");
    printf("  -g color	background color\n");
    printf("  -n name	set title part of PostScript output to name\n");
    printf("  -p name	name of the PostScript file to be overlaid\n");

    printf("TEXTYL Options: None\n");

    printf("TK Options:\n");
    printf("  -l dummyarg	landscape mode\n");
    printf("  -p dummyarg	portrait mode\n");
    printf("  -P		generate canvas of full page size instead of figure bounds\n");
    printf("  -z papersize	set the papersize (see man pages for available sizes)\n");

    printf("TPIC Options: None\n");
}

/* count primitive objects & create pointer array */
static int compound_dump(com, array, count, dev)
    F_compound		*com;
    struct obj_rec	*array;
    int			 count;
    struct driver	*dev;
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

int
gendev_objects(objects, dev)
    F_compound		*objects;
    struct driver	*dev;
{
	int	obj_count, rec_comp();
	int	status;
	struct	obj_rec *rec_array, *r; 

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
	  if (depth_filter(r))
	    (*(r->gendev))(r->obj);

	/* generate trailer */
	status = (*dev->end)();

	free(rec_array);

	return status;
}

int rec_comp(r1, r2)
    struct obj_rec	*r1, *r2;
{
	return (r2->depth - r1->depth);
}

/* null operation */
void gendev_null() {
    ;
}

/*
 * depth_options:
 *  +range[,range...]
 *  -range[,range...]
 *  where range is:
 *  d       include/exclude this depth
 *  d1:d2   include/exclude this range of depths
 */

depth_usage()
{
    fprintf(stderr,"%s: help for -D option:\n",prog);
    fprintf(stderr,"  -D +rangelist  means keep only depths in rangelist.\n");
    fprintf(stderr,"  -D -rangelist  means keep all depths but those in rangelist.\n");
    fprintf(stderr,"  rangelist can be a list of numbers or ranges of numbers, e.g.:\n");
    fprintf(stderr,"    10,40,55,60:70,99\n");
    exit(1);
}

depth_option(s)
char *s;
{
  struct depth_opts *d;

  switch (depth_op = *s++) {
    case '+':
    case '-':
	break;
    default:
	depth_usage();
  }
  
  for (d = depth_opt; depth_index < NUMDEPTHS && *s; ++depth_index, d++) {
    if (*s == ',') ++s;
    d->d1 = d->d2 = -1;
    d->d1 = strtol(s,&s,10);
    if (d->d1 <= 0) 
      depth_usage();
    switch(*s) {		/* what's the delim? */
    case ':':			/* parse a range */
      d->d2 = strtol(++s,&s,10);
      if (d->d2 < d->d1) 
	depth_usage();
      break;
    case ',':			/* just start the next one */
      ++s;
      break;
    }
  }
  if (depth_index >= NUMDEPTHS) {
    fprintf(stderr,"%s: Too many -D values!\n",prog);
    exit(1);
  }
}

depth_filter(r)
struct obj_rec *r;
{
  struct depth_opts *d;
  
  if (depth_index <= 0)		/* no filters were set up */
    return 1;
  for (d = depth_opt; d->d1 > 0; d++)
    if (d->d2 >= 0) {		/* it's a range comparison */
      if (r->depth >= d->d1 && r->depth <= d->d2)
	return (depth_op=='+')?1:0;
    } else {			/* it's a single-value comparison */
      if (d->d1 == r->depth)
	return (depth_op=='+')?1:0;
    }
  return (depth_op=='-')?1:0;
}

void
gs_broken_pipe(int sig)
{
  fprintf(stderr,"fig2dev: broken pipe (GhostScript aborted?)\n");
  fprintf(stderr,"command was: %s\n", gscom);
  exit(1);
}

