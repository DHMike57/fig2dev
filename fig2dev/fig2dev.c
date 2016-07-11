/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1991 by Micah Beck
 * Parts Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2002 by Brian V. Smith
 *
 * Any party obtaining a copy of these files is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and
 * documentation files (the "Software"), including without limitation the
 * rights to use, copy, modify, merge, publish and/or distribute copies of
 * the Software, and to permit persons who receive copies from any such
 * party to do so, with the only requirement being that this copyright
 * notice remain intact.
 *
 */


/*
 *	fig2dev.c: General Fig code translation program
 *
 * Changes:
 *
 * 2015-12-07 - Define booleans std_color_used[], arrows_used, arrow_used[].
 *		Modify for build with autoconf.
 *		Add help text for pict2e- and tikz-drivers. (Thomas Loimer)
 *
 */

#include "fig2dev.h"
#include <sys/file.h>
#include <locale.h>
#include "alloc.h"
#include "object.h"
#include "drivers.h"
#include "bound.h"
#include "read.h"
/* setmode() exists on DOS/Windows. It sets file mode to text or binary.
 * setmode() is declared in <io.h>, O_BINARY is declared in <fcntl.h>. */
#ifdef HAVE__SETMODE
#include <io.h>
#include <fcntl.h>
#endif

extern int	 fig_getopt(int nargc, char **nargv, char *ostr);
extern char	*optarg;
extern int	 optind;
static int	 parse_gridspec(char *string, float *numer, float *denom,
				float *spacing, int *nchrs);
static void	 grid_usage(void);
static int	 gendev_objects(F_compound *objects, struct driver *dev);

static void	 help_msg(void);
static void	 depth_option(char *s);

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
char	Usage[] =
  "Usage:\n %s -hV\n %s -L language [-s size] [-m scale] [-j] [input [output]]\n";
bool support_i18n = false;
#else
char	Usage[] =
 "Usage:\n %s -hV\n %s -L language [-s size] [-m scale] [input [output]]\n";
#endif  /* I18N */

char	Err_badarg[] = "Argument -%c unknown to %s driver.";
char	Err_mem[] = "Running out of memory.";

char	*prog;
char	*from = NULL, *to = NULL;
char	*name = NULL;
double	font_size = 0.0;
double	mag = 1.0;
double	fontmag = 1.0;
FILE	*tfp = NULL;

double	ppi;			/* Fig file resolution (e.g. 1200) */
int	llx = 0, lly = 0, urx = 0, ury = 0;
bool	landscape;
bool	center;
bool	orientspec = false;	/* set if the user specs. the orientation */
bool	centerspec = false;	/* set if the user specs. the justification */
bool	magspec = false;	/* set if the user specs. the magnification */
bool	transspec = false;	/* set if the user specs. the GIF transparent color */
bool	multispec = false;	/* set if the user specs. multiple pages */
bool	paperspec = false;	/* set if the user specs. the paper size (-z) */
bool	boundingboxspec = false;/* set if the user specs. the bounding box (-B or -R) */
bool maxdimspec = false;	/* set fi the user specs. the max size of the figure (-Z) */
bool	pats_used, pattern_used[NUMPATTERNS];
bool	std_color_used[NUM_STD_COLS];
bool	arrows_used, arrow_used[NUMARROWS];
bool	multi_page = false;	/* multiple page option for PostScript */
bool	overlap = false;	/* overlap pages in multiple page output */
bool	metric;			/* true if file specifies Metric */
char	gif_transparent[20]="\0"; /* GIF transp color hex name (e.g. #ff00dd) */
char	papersize[20];		/* paper size */
char	boundingbox[64];	/* boundingbox */
float	THICK_SCALE;		/* convert line thickness from screen res. */
					/* calculated in read_objects() */
char	lang[40];		/* selected output language */
RGB	background;		/* background (if specified by -g) */
bool	bgspec = false;		/* flag to say -g was specified */
float	grid_minor_spacing=0.0;	/* grid minor spacing (if any) */
float	grid_major_spacing=0.0;	/* grid major spacing (if any) */
float	mult;			/* multiplier for grid spacing */
char	gscom[1000];		/* to build up a command for ghostscript */
float	max_dimension;		/* if the user chooses max figure dimension (-Z) */

bool	psencode_header_done = false; /* if we have already emitted PSencode header */
bool	transp_header_done = false;   /* if we have already emitted transparent image header */
bool	grayonly = false;	/* convert colors to grayscale (-N option) */

struct obj_rec {
	void (*gendev)();
	char *obj;
	int depth;
};

#define NUMDEPTHS 100

struct depth_opts {
		int d1,d2;
	} depth_opt[NUMDEPTHS+1];
int	depth_index = 0;
char	depth_op;		/* '+' for skip all but those listed */
bool	adjust_boundingbox = false;

struct paperdef paperdef[] =
{
	{"Letter", 612, 792},	/*  8.5" x 11" */
	{"Legal", 612, 1008},	/*  8.5" x 14" */
	{"Tabloid", 792, 1224},	/*   11" x 17" */
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
	{"A4",  595, 842},		/*  210 mm x  297 mm */
	{"A3",  842, 1190},		/*  297 mm x  420 mm */
	{"A2", 1190, 1684},		/*  420 mm x  594 mm */
	{"A1", 1684, 2383},		/*  594 mm x  841 mm */
	{"A0", 2383, 3370},		/*  841 mm x 1189 mm */
	{"B10",  91,  127},		/*   32 mm x   45 mm */
	{"B9",  127,  181},		/*   45 mm x   64 mm */
	{"B8",  181,  258},		/*   64 mm x   91 mm */
	{"B7",  258,  363},		/*   91 mm x  128 mm */
	{"B6",  363,  516},		/*  128 mm x  182 mm */
	{"B5",  516,  729},		/*  182 mm x  257 mm */
	{"B4",  729, 1032},		/*  257 mm x  364 mm */
	{"B3", 1032, 1460},		/*  364 mm x  515 mm */
	{"B2", 1460, 2064},		/*  515 mm x  728 mm */
	{"B1", 2064, 2920},		/*  728 mm x 1030 mm */
	{"B0", 2920, 4127},		/* 1030 mm x 1456 mm */
	{NULL, 0, 0}
};

void
put_msg(char *fmt, ...)
{
    va_list argptr;
        va_start(argptr, fmt);
        vfprintf(stderr, fmt, argptr);
        va_end(argptr);
        fputc('\n', stderr);
}

/* all option letters must be in this string */
/* not in this string: HJQu and non-alphabetic chars */

#ifdef I18N
#define ARGSTRING	"AaB:b:C:cD:d:E:eFf:G:g:hI:i:jKkL:l:Mm:Nn:OoPp:q:R:rS:s:Tt:UVvWwX:x:Y:y:Z:z:?"
#else
#define ARGSTRING	"AaB:b:C:cD:d:E:eFf:G:g:hI:i:KkL:l:Mm:Nn:OoPp:q:R:rS:s:Tt:UVvWwX:x:Y:y:Z:z:?"
#endif

/* Options with `continue;' are not passed to drivers;
 * Therefore, only -L and -G remain to be cared for in drivers.
 * -L is needed in genepic and genmp while in genbitmaps, lang is used.
 * (TODO: either use lang in all, or -L in all)
 * -G is needed in genpstricks for an error message only */

void
get_args(int argc, char *argv[])
{
	int	 c, i, nvals, nchars;
	char	*grid, *p;
	float	 numer, denom;

	prog = *argv;
	if (argc == 1)
	    fprintf(stderr,Usage,prog,prog);
	/* sum of all arguments */
	while ((c = fig_getopt(argc, argv, ARGSTRING)) != EOF) {

	  /* global (all drivers) option handling */
	  switch (c) {

		case 'h':	/* print version message for -h too */
		case 'V':
		    printf("fig2dev Version %s\n", PACKAGE_VERSION);
		    if (c == 'h')
			help_msg();
		    exit(0);
		    break;

	        case 'D':	                /* depth filtering */
		    depth_option(optarg);
		    /* options with "continue" are not passed to the driver */
		    continue;

	        case 'K':
		    /* adjust bounding box according to selected depth range
		       given with '-D RANGE' option above */
		    adjust_boundingbox = true;
		    continue;

		case 'G':			/* Grid */
		    /* if major AND minor are specified */
		    if ((p = strchr(optarg,':'))) {
			/* parse minor grid spec first */
			if (p != optarg) {
			    if ((nvals=parse_gridspec(optarg, &numer, &denom,
				&grid_minor_spacing, &nchars)) == 2)
				    grid_minor_spacing = numer/denom;
			}
			/* now parse major grid spec */
			if ((nvals=parse_gridspec(p+1, &numer, &denom,
			     &grid_major_spacing, &nchars)) == 2)
				grid_major_spacing = numer/denom;
		    } else {
			/* only minor grid specified */
			if ((nvals=parse_gridspec(optarg, &numer, &denom,
			     &grid_minor_spacing, &nchars)) == 2)
				grid_minor_spacing = numer/denom;
			p = optarg-1;
		    }
		    /* now parse any units after grid spacing */
		    if (nchars < strlen(p+1)) {
			grid = p+nchars+1;
			if ((strcmp(grid,"i") && strcmp(grid,"in") && strcmp(grid,"inch") &&
				 strcmp(grid,"f") && strcmp(grid,"ft") && strcmp(grid,"feet") &&
				 strcmp(grid,"c") && strcmp(grid,"cm") &&
				 strcmp(grid,"mm") && strcmp(grid,"m") )) {
			    grid_usage();
			    grid_major_spacing = grid_minor_spacing = 0.0;
			    break;
			}
			if (!strcmp(grid,"i") || !strcmp(grid,"in") || !strcmp(grid,"in"))
			    mult =  1.0;
			else if (!strcmp(grid,"f") || !strcmp(grid,"ft") || !strcmp(grid,"feet"))
			    mult = 12.0;
			else if (strcmp(grid,"c") == 0 || strcmp(grid,"cm") == 0)
			    mult = 1.0/2.54;
			else if (strcmp(grid,"mm") == 0)
			    mult = 1.0/25.4;
			else if (strcmp(grid,"m") == 0)
			    mult = 1.0/0.0254;
		    } else {
			mult = 1.0;	/* no units, 1:1 */
		    }
		    break;	/* error message given in genpstricks.c */

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
		    break;	/* needed in genepic.c, genmp.c */

		case 'm':			/* set magnification */
		    fontmag = mag = atof(optarg);
		    magspec = true;		/* user-specified */
		    continue;

		case 's':			/* set default font size */
		    font_size = atof(optarg);
		    /* max size is checked in respective drivers */
		    if (font_size <= 0.0)
			font_size = DEFAULT_FONT_SIZE;
		    continue;

		case 'Z':			/* scale to requested size (inches/cm) */
		    maxdimspec = true;
		    max_dimension = atof(optarg);
		    continue;

#ifdef I18N
		case 'j':
		    support_i18n = true;
		    continue;
#endif /* I18N */

		case '?':			/* usage		*/
			fprintf(stderr,Usage,prog,prog);
			exit(1);
	    }

	    /* pass options through to driver */
	    if (!dev) {
		fprintf(stderr, "No graphics language specified.\n");
		exit(1);
	    }
	    dev->option(c, optarg);
	}
	/* adjust font size after option loop to make sure we have any -m first,
	   which affects fontmag */
	font_size = font_size * fontmag;

	if (!dev) {
		fprintf(stderr, "No graphics language specified.\n");
		exit(1);
	}

	/* make sure user doesn't specify both mag and max dimension */
	if (magspec && maxdimspec) {
		fprintf(stderr, "Must specify only one of -m (magnification) and -Z (max dimension).\n");
		exit(1);
	}

	if (optind < argc)
		from = argv[optind++];	/*  from file  */
	if (optind < argc)
		to   = argv[optind];	/*  to file    */
}

int
parse_gridspec(char *string, float *numer,float *denom, float *spacing,
		int *nchrs)
{
	int  numvals;

	if ((numvals=sscanf(string,"%f/%f%n",numer,denom,nchrs)) == 2) {
	    if (*numer < 0.0 || *denom < 0.0 || *denom == 0.0)
		return 0;
	    else
		return numvals;
	} else if ((numvals=sscanf(string,"%f%n",spacing,nchrs)) == 1) {
	    if (*spacing < 0.0 || *spacing == 0.0)
		return 0;
	    else
		return numvals;
	}

	grid_usage();
	return 0;
}

static void
grid_usage(void)
{
     fprintf(stderr,"Can't parse grid spec. Format is -G [minor_tick][:major_tick]unit ");
     fprintf(stderr,"e.g. -G .5:2cm or -G 1/16:1/2in\n");
     fprintf(stderr,"Allowable units are: ");
     fprintf(stderr,"'i', 'in', 'ft', 'feet', 'c', 'cm', 'mm', 'm'.");
     fprintf(stderr,"  Ignoring grid.\n");
}

int
main(int argc, char *argv[])
{
	F_compound	objects;
	int		status;

	setlocale(LC_CTYPE, "");
#ifdef HAVE__SETMODE
	_setmode(1,O_BINARY); /* stdout is binary */
#endif

	/* get the options */
	get_args(argc, argv);

	/* read the Fig file */

	if (from)
	    status = read_fig(from, &objects);
	else	/* read from stdin */
	    status = readfp_fig(stdin, &objects);

	/* multiply grid spacing by unit and scale to get FIG units */
	grid_minor_spacing = mult * grid_minor_spacing * ppi;
	grid_major_spacing = mult * grid_major_spacing * ppi;

	if (status != 0) {
	    if (from)
		read_fail_message(from, status);
	    exit(1);
	}

	if (to == NULL)
	    tfp = stdout;
	else {
	    if (strstr(to, ".fig") == to + strlen(to)-4) {
		fprintf(stderr,"Outfile is a .fig file, aborting\n");
		exit(1);
	    }
	    if ((tfp = fopen(to, "wb")) == NULL) {
		fprintf(stderr, "Couldn't open %s\n", to);
		exit(1);
	    }
	}

	/* Compute bounding box of objects, supressing texts if indicated */
	compound_bound(&objects, &llx, &lly, &urx, &ury, dev->text_include);

	/* make sure bounding box has width and height (if there is only latex special
	 * text, it may be 0 width */
	if (urx-llx == 0)
	     urx = llx+100;
	if (ury-lly == 0)
	     ury = lly+100;

	/* if user has specified the maximum dimension, adjust mag to produce that */
	if (maxdimspec) {
	    float maxdim;

	    if (metric)
		max_dimension /= 2.54;
	    /* find larger of width and height */
	    maxdim = MAX(urx - llx, ury - lly)/ppi;
	    /* override any mag specified in the Fig file */
	    mag = max_dimension/maxdim;
	}

	/* If metric, adjust scale for difference between FIG PIX/CM (450) and actual (472.44)
	   NOTE: we don't adjust font sizes by this amount because they are in points,
	   not inches or cm */
	if (metric)
		mag *= 80.0/76.2;

	status = gendev_objects(&objects, dev);
	if ((tfp != stdout) && (tfp != 0))
	    (void)fclose(tfp);
	exit(status);
}

void
help_msg(void)
{
    int i;

    puts("General Options (all drivers):");
    puts("  -L language	choose output language (this must be first)");
    /* display available languages - 23/01/90 */
    printf("                Available languages are:");
    for (i=0; *drivers[i].name; i++) {
	if (i%9 == 0)
	    printf("\n\t\t  ");
	printf("%s ",drivers[i].name);
    }
    printf("\n");
    puts("  -h		print this message, fig2dev version number and exit");
    puts("  -V		print fig2dev version number and exit");
    puts("  -D +/-list	include or exclude depths listed");
    puts("  -K		adjust bounding box according to selected depths" );
    puts("		        given with '-D +/-list' option.");
    puts("  -G minor[:major][unit]    draw light gray grid with thin/thick lines at");
    puts("		  minor/major units (e.g., -G .25:1cm).");
#ifdef I18N
    puts("  -j		enable i18n facility");
#endif
    puts("  -m mag	set magnification.  This may not be used with the -Z option.");
    puts("  -s size	set default font size in points");
    puts("  -Z maxdim	Scale the figure so that the maximum dimension (width or height)");
    puts("		  is maxdim inches/cm.  This may not be used with the -m option.");
    printf("\n");

    puts("--------------------------------------------------------------------------------");
    puts("Vector formats:");
    puts("CGM Options:");
    puts("  -b dummyarg	generate binary output (dummy argument required after \"-b\")");
    puts("  -r		position arrowheads for CGM viewers that use rounded arrowheads");

    puts("EPIC Options:");
    puts("  -A scale	scale arrowheads by dividing their size by scale");
    puts("  -E num	set encoding for text translation (0 = no translation,");
    puts("		  1 = ISO-8859-1, 2 = ISO-8859-2)");
#ifdef NFSS
    puts("  -F		don't set font family/series/shape, so you can set it from latex");
    puts("  -f font	set default font");
#endif /* NFSS */
    puts("  -l lwidth	use \"thicklines\" when width of line is > lwidth");
    puts("  -P		generate a complete LaTeX file");
#ifdef NFSS
    puts("  -R dummyarg	allow rotated text (uses \\rotatebox)");
#endif /* NFSS */
    puts("  -S scale	scale figure");
    puts("  -t stretch	set the stretch of dashed lines (default = 30)");
    puts("  -v		include comments in the output");
    puts("  -W		enable variable line width");
    puts("  -w		disable variable line width");

    puts("EPS (Encapsulated PostScript) Options:");
    puts("  -A		add ASCII (EPSI) preview");
    puts("  -B \"Wx [Wy X0 Y0]\"    force width, height and origin in absolute coordinates");
    puts("  -b width	specify width of blank border around figure (points, 1/72 inch)");
    puts("  -C dummyarg	add color TIFF preview (for Microsoft apps)");
    puts("  -F		use correct font sizes (points instead of 1/80inch)");
    puts("  -f font	set default font");
    puts("  -g color	background color");
    puts("  -N		convert all colors to grayscale");
    puts("  -n name	set title part of PostScript output to name");
    puts("  -R \"Wx [Wy X0 Y0]\"    force width, height and origin in relative");
    puts("		  coordinates (relative to lower-left of figure bounds)");
    puts("  -T		add monochrome TIFF preview (for Microsoft apps)");

    puts("GBX (Gerber, RS-247-X)  Options:");
    puts("  -d [mm|in]	Output dimensions assumed to be millimeters or inches.");
    puts("  -p [pos|neg]	Image Polarity - positive results in lines being drawn.");
    puts("		Negative results in lines erasing background.");

    puts("  -g <x scale>x<y scale>+<x offset>+<y offset>    Scale and shift output.");
    puts("		  Typically, use -g 1x-1+0+0, mirroring about the x axis.");
    puts("  -f <n digits>.<n digits>    Digits of precision before and");
    puts("		  after the implied decimal point.");
    puts("  -i [on|off]	Controls the output of comments. On, by default.");

    puts("IBM-GL Options:");
#ifdef A4
    puts("  -a		select ANSI A paper size instead of the default ISO A4");
#else
    puts("  -a		select ISO A4 paper size instead of the default ANSI A");
#endif
#ifdef IBMGEC
    puts("  -c		generate instructions for IBM 6180 plotter without IBM-GEC");
#else
    puts("  -c		generate instructions for IBM 6180 plotter with IBM-GEC");
#endif
    puts("  -d xll,yll,xur,yur    restrict plotting to area specified by coords");
    puts("  -f fontfile	load text character specs from table in file");
    puts("  -k		precede output with PCL command to use HP/GL");
    puts("  -l pattfile	load patterns for pattern fill from file");
    puts("  -m mag,x0,y0	magnification with optional offset in inches");
    puts("  -P		rotate figure to portrait (default is landscape)");
    puts("  -p penfile	load plotter pen specs from file");
    puts("  -S speed	set pen speed in cm/sec");
    puts("  -v		print figure upside-down in portrait or backwards in landscape");

    puts("LaTeX Options:");
    puts("  -d dmag	set separate magnification for length of line dashes to dmag");
    puts("  -E num	set encoding for text translation (0 = no translation,");
    puts("		  1 = ISO-8859-1, 2 = ISO-8859-2)");
    puts("  -f font	set default font");
    puts("  -l lwidth	set threshold between thin and thick lines to lwidth");
    puts("  -v		verbose mode");

    puts("MAP (HTML image map) Options:");
    puts("  -b width	specify width of blank border around figure (points, 1/72 inch)");

    puts("METAFONT Options:");
    puts("  -C code	specifies the starting METAFONT font code");
    puts("  -n name	name to use in the output file");
    puts("  -p pen_mag	linewidth magnification compared to the original figure");
    puts("  -t top	specifies the top of the coordinate system");
    puts("  -x xmin	specifies minimum x coordinate of figure (inches)");
    puts("  -y ymin	specifies minimum y coordinate of figure (inches)");
    puts("  -X xmax	specifies maximum x coordinate of figure (inches)");
    puts("  -Y xmax	specifies maximum y coordinate of figure (inches)");

    puts("METAPOST Options:");
    puts("  -i		fileincludes file content via \\input-command");
    puts("  -I		fileadds file content as additional header");
    puts("  -o		old mode (no latex)");
    puts("  -p number	adds the line \"prologues:=number\"");

    puts("PIC Options:");
    puts("  -p ext	enables certain PIC extensions (see man pages)");

    puts("PICTeX Options:");
    puts("  -a		don't output user's login name (anonymous)");
    puts("  -E num	set encoding for text translation (0 = no translation,");
    puts("		  1 = ISO-8859-1, 2 = ISO-8859-2)");
    puts("  -f font	set default font");

    puts("PICT2E Options:");
    puts("  -b width	specify width of blank border around figure (points, 1/72 inch)");
    puts("  -C num	do not issue color commands for color num");
    puts("\t\t  (0 = black, 1 = red, etc.)");
    puts("  -e		do not try to be compatible with epic/eepic");
    puts("  -E num	set encoding for text translation (0 = no translation,");
    puts("		  1 = ISO-8859-1, 2 = ISO-8859-2)");
    puts("  -F		do not set font family/series/shape");
    puts("  -f font	set default font");
    puts("  -i dir	prepend the string \"dir\" to included graphics files");
    puts("  -o		do not set the font size");
    puts("  -O		do not quote characters special to TeX/LaTeX");
    puts("		  (effectively, set the text-flag \"special-text\" for all text)");
    puts("  -P		pagemode, generate stand-alone LaTeX-file");
    puts("  -R num	use LaTeX-arrows for arrow-type num");
    puts("  -r		use LaTeX-arrows for all arrows");
    puts("  -T		only use TeX fonts (even when PS-fonts are specified)");
    puts("  -v		verbose mode, include comments in the output");
    puts("  -w		remove suffix from included graphics-files");

    puts("PostScript Options:");
    puts("  -a		don't output user's login name (anonymous)");
    puts("  -A		add ASCII (EPSI) preview");
    puts("  -b width	specify width of blank border around figure (1/72 inch)");
    puts("  -C dummyarg	add color TIFF preview (for Microsoft apps)");
    puts("  -c		center figure on page");
    puts("  -e		put figure at left edge of page");
    puts("  -F		use correct font sizes (points instead of 1/80inch)");
    puts("  -f font	set default font");
    puts("  -g color	background color");
    puts("  -l dummyarg	landscape mode (dummy argument required after \"-l\")");
    puts("  -M		generate multiple pages for large figure");
    puts("  -N		convert all colors to grayscale");
    puts("  -n name	set title part of PostScript output to name");
    puts("  -O		overlap pages in multiple page mode (-M)");
    puts("  -p dummyarg	portrait mode (dummy argument required after \"-p\")");
    puts("  -T		add monochrome TIFF preview (for Microsoft apps)");
    puts("  -x offset	shift figure left/right by offset units (1/72 inch)");
    puts("  -y offset	shift figure up/down by offset units (1/72 inch)");
    puts("  -z papersize	set the papersize (see man pages for available sizes)");

    puts("PDF Options:");
    puts("   All of the EPS options plus:\n");
    puts("  -a		don't output user's login name (anonymous)");
    puts("  -b width	specify width of blank border around figure (1/72 inch)");
    puts("  -F		use correct font sizes (points instead of 1/80inch)");
    puts("  -g color	background color");
    puts("PSTricks Options:");
    puts("  -f fontname	set default font");
    puts("  -G dummyarg	draw pstricks standard grid (sizes ignored)");
    puts("  -l weight	set line weight factor 0 to 2.0 (default 0.5 matches PS driver)");
    puts("  -P		generate complete {document}; scale to fit one page");
    puts("  -p dir	turn on auto picture conversion to EPS; dir is repository");
    puts("  -R 0|1|2	arrow style (default 0 = Fig, 1 = PST with Fig sizes,");
    puts("		   2 = PST default, ignore Fig sizes)");
    puts("  -S scale	hard scale factor");
    puts("  -t version	set PSTricks version");
    puts("  -v		verbose warnings and comments in output file");
    puts("  -x marginsize	add margin on left and right (default is tight bounding box)");
    puts("  -y marginsize	add margin on over and under (default is tight bounding box)");
    puts("  -z 0|1|2	font handling (default 0=full, 1=size only, 2=none,PST default)");

    puts("  -M		generate multiple pages for large figure");
    puts("  -N		convert all colors to grayscale");
    puts("  -O		overlap pages in multiple page mode (-M)");
    puts("  -S dummyarg	generate single page");

    puts("PSTEX Options:");
    puts("  -b width	specify width of blank border around figure (1/72 inch)");
    puts("  -F		use correct font sizes (points instead of 1/80inch)");
    puts("  -g color	background color");
    puts("  -n name	set title part of PostScript output to name");

    puts("PSTEX_T Options:");
    puts("  -b width	specify width of blank border around figure (1/72 inch)");
    puts("  -E num	set encoding for text translation (0 = no translation,");
    puts("		  1 = ISO-8859-1, 2 = ISO-8859-2)");
#ifdef NFSS
    puts("  -F		don't set font family/series/shape, so you can");
    puts("		  set it from latex");
#endif /* NFSS */
    puts("  -p name	name of the PostScript file to be overlaid");

    puts("SHAPE (ShapePar driver) Options:");
    puts("  -n name	Set basename of the macro");
    puts("		  (e.g. \"face\" gives faceshape and facepar)");
    puts("Tcl/Tk (tk) and Perl/Tk (ptk) Options:");
    puts("  -l dummyarg	landscape mode (dummy argument required after \"-l\")");
    puts("  -p dummyarg	portrait mode (dummy argument required after \"-p\")");
    puts("  -P		generate canvas of full page size instead of figure bounds");
    puts("  -z papersize	set the papersize (see man pages for available sizes)");

    puts("TIKZ Options:");
    puts("  -b width	specify width of blank border around figure (1/72 inch)");
    puts("  -C num	do not issue color commands for color num");
    puts("		  (0 = black, 1 = red, etc.)");
    puts("  -e		do not try to be compatible with epic/eepic");
    puts("  -E num	set encoding for text translation (0 = no translation,");
    puts("		  1 = ISO-8859-1, 2 = ISO-8859-2)");
    puts("  -F		do not set font family/series/shape");
    puts("  -i dir	prepend the string \"dir\" to included graphics files");
    puts("  -o		do not set the font size");
    puts("  -O		do not quote characters special to TeX/LaTeX");
    puts("		  (effectively, set the text-flag \"special-text\" for all text)");
    puts("  -P		pagemode, generate stand-alone LaTeX-file");
    puts("  -T		only use TeX fonts (even when PS-fonts are specified)");
    puts("  -v		verbose mode, include comments in the output");
    puts("  -w		remove suffix from included graphics-files");

    puts("--------------------------------------------------------------------------------");
    puts("Bitmap formats:");
    puts("Options common to all bitmap formats:");
    puts("  -b width	specify width of blank border around figure (pixels)");
    puts("  -F		use correct font sizes (points instead of 1/80inch)");
    puts("  -g color	background color");
    puts("  -N		convert all colors to grayscale");
    puts("  -S smooth	specify smoothing factor [1=none, 2=some 4=more]");

    puts("GIF Options:");
    puts("  -t color	specify GIF transparent color in hexadecimal (e.g. #ff0000=red)");

    puts("JPEG Options:");
    puts("  -q quality	specify image quality factor (0-100)");

}

/* count primitive objects & create pointer array */
static int compound_dump(F_compound *com, struct obj_rec *array,
			int count, struct driver *dev)
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
gendev_objects(F_compound *objects, struct driver *dev)
{
	int	obj_count, rec_comp();
	int	status;
	struct	obj_rec *rec_array, *r;

	/* dump object pointers to an array */
	obj_count = compound_dump(objects, 0, 0, dev);
	if (!obj_count) {
	    fprintf(stderr, "fig2dev: No objects in Fig file\n");
	    return -1;
	}
	rec_array = (struct obj_rec *)malloc(obj_count*sizeof(struct obj_rec));
	(void)compound_dump(objects, rec_array, 0, dev);

	/* sort object array by depth */
	qsort(rec_array, obj_count, sizeof(struct obj_rec), rec_comp);

	/* generate header */
	(*dev->start)(objects);

	/* draw any grid specified */
	(*dev->grid)(grid_major_spacing, grid_minor_spacing);

	/* generate objects in sorted order */
	for (r = rec_array; r<rec_array+obj_count; r++)
	  if (depth_filter(r->depth))
	    (*(r->gendev))(r->obj);

	/* generate trailer */
	status = (*dev->end)();

	free(rec_array);

	return status;
}

int rec_comp(struct obj_rec *r1, struct obj_rec *r2)
{
	return (r2->depth - r1->depth);
}

/* null operations */
void
gendev_null(void)
{ ; }

void
gendev_nogrid(float major, float minor)
{ ; }

/*
 * depth_options:
 *  +range[,range...]
 *  -range[,range...]
 *  where range is:
 *  d       include/exclude this depth
 *  d1:d2   include/exclude this range of depths
 */

static void
depth_usage(void)
{
    fprintf(stderr,"%s: help for -D option:\n",prog);
    fprintf(stderr,"  -D +rangelist  means keep only depths in rangelist.\n");
    fprintf(stderr,"  -D -rangelist  means keep all depths but those in rangelist.\n");
    fprintf(stderr,"  Rangelist can be a list of numbers or ranges of numbers, e.g.:\n");
    fprintf(stderr,"    10,40,55,60:70,99\n");
    exit(1);
}

void
depth_option(char *s)
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
	    if (d->d1 < 0)
		depth_usage();
	    switch(*s) {		/* what's the delim? */
	    case ':':			/* parse a range */
		d->d2 = strtol(s+1,&s,10);
		if (d->d2 < d->d1)
		    depth_usage();
		break;
	    case ',':			/* just start the next one */
		s++;
		break;
	    }
	}
	if (depth_index >= NUMDEPTHS) {
	    fprintf(stderr,"%s: Too many -D values!\n",prog);
	    exit(1);
	}
	d->d1 = -1;
}

int
depth_filter(int obj_depth)
{
	struct depth_opts *d;

	if (depth_index <= 0)		/* no filters were set up */
	    return 1;
	for (d = depth_opt; d->d1 >= 0; d++)
	    if (d->d2 >= 0) {		/* it's a range comparison */
		if (obj_depth >= d->d1 && obj_depth <= d->d2)
		    return (depth_op=='+')? 1:0;
	    } else {			/* it's a single-value comparison */
		if (d->d1 == obj_depth)
		    return (depth_op=='+')? 1:0;
	    }
	return (depth_op=='-')? 1:0;
}

void
gs_broken_pipe(int sig)
{
	fprintf(stderr,"fig2dev: broken pipe (GhostScript aborted?)\n");
	fprintf(stderr,"command was: %s\n", gscom);
	exit(1);
}
