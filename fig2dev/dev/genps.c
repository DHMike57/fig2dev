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
 *	genps.c: PostScript driver for fig2dev
 *
 *	Modified by Herbert Bauer to support ISO-Characters,
 *	multiple page output, color mode etc.
 *	heb@regent.e-technik.tu-muenchen.de
 *
 *	Modified by Eric Picheral to support the whole set of ISO-Latin-1
 *	Modified by Herve Soulard to allow non-iso coding on special fonts
 *	Herve.Soulard@inria.fr (8 Apr 1993)

*/

#include <sys/param.h>
#if defined(hpux) || defined(SYSV) || defined(BSD4_3) || defined(SVR4)
#include <sys/types.h>
#endif
#include <sys/file.h>
#include <pwd.h>
#include <errno.h>

#include "fig2dev.h"
#include "figure.h"
#include "object.h"
#include "psfonts.h"
#include <string.h>
#include <time.h>

/* for the xpm package */
#ifdef USE_XPM
#include <xpm.h>
int	XpmReadFileToXpmImage();
#endif /* USE_XPM */

#ifdef I18N
#include <stdlib.h>
extern Boolean support_i18n;  /* enable i18n support? */
static Boolean enable_composite_font = False;
#endif /* I18N */

/* for the version nubmer */
#include "../../patchlevel.h"

/* include the PostScript preamble, patterns etc */
#include "genps.h"

typedef struct _point
{
    int x,y;
} Point;

#define		POINT_PER_INCH		72
#define		ULIMIT_FONT_SIZE	300

#define		min(a, b)		(((a) < (b)) ? (a) : (b))

void		gen_ps_eps_option();

Boolean		epsflag = False;	/* to distinguish PS and EPS */

int		pagewidth = -1;
int		pageheight = -1;
int		xoff=0;
int		yoff=0;
static int	coord_system;
static int	resolution;
static double	cur_thickness = 0.0;
static int	cur_joinstyle = 0;
static int	cur_capstyle = 0;
int		pages;
int		no_obj = 0;
static int	border_margin = 0;

/* arrowhead arrays */
Point		bpoints[50], fpoints[50];
int		nbpoints, nfpoints;
int		fpntx1, fpnty1;	/* first point of object */
int		fpntx2, fpnty2;	/* second point of object */
int		lpntx1, lpnty1;	/* last point of object */
int		lpntx2, lpnty2;	/* second-to-last point of object */

static		arc_tangent();
static		fill_area();
static		clip_arrows();
static		calc_arrow();
static		draw_arrow();
static		iso_text_exist();
static		encode_all_fonts();
static		ellipse_exist();
static		approx_spline_exist();
static double	compute_angle();

#define SHADEVAL(F)	1.0*(F)/(NUMSHADES-1)
#define TINTVAL(F)	1.0*(F-NUMSHADES+1)/NUMTINTS

/* define the standard 32 colors */

struct	_rgb {
	float r, g, b;
	}
    rgbcols[32] = {
	{0.00, 0.00, 0.00},
	{0.00, 0.00, 1.00},
	{0.00, 1.00, 0.00},
	{0.00, 1.00, 1.00},
	{1.00, 0.00, 0.00},
	{1.00, 0.00, 1.00},
	{1.00, 1.00, 0.00},
	{1.00, 1.00, 1.00},
	{0.00, 0.00, 0.56},
	{0.00, 0.00, 0.69},
	{0.00, 0.00, 0.82},
	{0.53, 0.81, 1.00},
	{0.00, 0.56, 0.00},
	{0.00, 0.69, 0.00},
	{0.00, 0.82, 0.00},
	{0.00, 0.56, 0.56},
	{0.00, 0.69, 0.69},
	{0.00, 0.82, 0.82},
	{0.56, 0.00, 0.00},
	{0.69, 0.00, 0.00},
	{0.82, 0.00, 0.00},
	{0.56, 0.00, 0.56},
	{0.69, 0.00, 0.69},
	{0.82, 0.00, 0.82},
	{0.50, 0.19, 0.00},
	{0.63, 0.25, 0.00},
	{0.75, 0.38, 0.00},
	{1.00, 0.50, 0.50},
	{1.00, 0.63, 0.63},
	{1.00, 0.75, 0.75},
	{1.00, 0.88, 0.88},
	{1.00, 0.84, 0.00}
    };

/* define the fill patterns */

char	*fill_def[NUMPATTERNS] = {
		FILL_PAT01,FILL_PAT02,FILL_PAT03,FILL_PAT04,
		FILL_PAT05,FILL_PAT06,FILL_PAT07,FILL_PAT08,
		FILL_PAT09,FILL_PAT10,FILL_PAT11,FILL_PAT12,
		FILL_PAT13,FILL_PAT14,FILL_PAT15,FILL_PAT16,
		FILL_PAT17,FILL_PAT18,FILL_PAT19,FILL_PAT20,
		FILL_PAT21,FILL_PAT22,
	};

int	patmat[NUMPATTERNS][2] = {
	16,  -8,
	16,  -8,
	16,  -8,
	16, -16,
	16, -16,
	16, -16,
	16,  16,
	16, -16,
	16,  -8,
	 8, -16,
	16, -16,
	24, -24,
	24, -24,
	24, -24,
	24, -24,
	16,  -8,
	 8,  -8,
	16, -16,
	30, -18,
	16, -16,
	16,  -8,
	 8, -16,
	};

/************** ARRAY FOR ARROW SHAPES **************/ 

struct _fpnt { 
		double x,y;
	};
struct _arrow_shape {
		int numpts;
		int tipno;
		double tipmv;
		struct _fpnt points[6];
	};

#define NUM_ARROW_TYPES 21
static struct _arrow_shape arrow_shapes[NUM_ARROW_TYPES+1] = {
		   /* number of points, index of tip, {datapairs} */
		   /* first point must be upper-left point of tail, then tip */

		   /* type 0 */
		   { 3, 1, 2.15, {{-1,0.5}, {0,0}, {-1,-0.5}}},
		   /* place holder for what would be type 0 filled */
		   { 0 },
		   /* type 1 simple triangle */
		   { 4, 1, 2.1, {{-1.0,0.5}, {0,0}, {-1.0,-0.5}, {-1.0,0.5}}},
		   /* type 1 filled simple triangle*/
		   { 4, 1, 2.1, {{-1.0,0.5}, {0,0}, {-1.0,-0.5}, {-1.0,0.5}}},
		   /* type 2 concave spearhead */
		   { 5, 1, 2.6, {{-1.25,0.5},{0,0},{-1.25,-0.5},{-1.0,0},{-1.25,0.5}}},
		   /* type 2 filled concave spearhead */
		   { 5, 1, 2.6, {{-1.25,0.5},{0,0},{-1.25,-0.5},{-1.0,0},{-1.25,0.5}}},
		   /* type 3 convex spearhead */
		   { 5, 1, 1.5, {{-0.75,0.5},{0,0},{-0.75,-0.5},{-1.0,0},{-0.75,0.5}}},
		   /* type 3 filled convex spearhead */
		   { 5, 1, 1.5, {{-0.75,0.5},{0,0},{-0.75,-0.5},{-1.0,0},{-0.75,0.5}}},
		   /* type 4 diamond */
		   { 5, 1, 1.15, {{-0.5,0.5},{0,0},{-0.5,-0.5},{-1.0,0},{-0.5,0.5}}},
		   /* type 4 filled diamond */
		   { 5, 1, 1.15, {{-0.5,0.5},{0,0},{-0.5,-0.5},{-1.0,0},{-0.5,0.5}}},
		   /* type 5 circle - handled in code */
		   { 0, 0, 0.0 }, { 0, 0, 0.0 },
		   /* type 6 half circle - handled in code */
		   { 0, 0, -1.0 }, { 0, 0, -1.0 },
		   /* type 7 square */
		   { 5, 1, 0.0, {{-1.0,0.5},{0,0.5},{0,-0.5},{-1.0,-0.5},{-1.0,0.5}}},
		   /* type 7 filled square */
		   { 5, 1, 0.0, {{-1.0,0.5},{0,0.5},{0,-0.5},{-1.0,-0.5},{-1.0,0.5}}},
		   /* type 8 reverse triangle */
		   { 4, 1, 0.0, {{-1.0,0},{0,0.5},{0,-0.5},{-1.0,0}}},
		   /* type 8 filled reverse triangle */
		   { 4, 1, 0.0, {{-1.0,0},{0,0.5},{0,-0.5},{-1.0,0}}},
		   /* type 9a "wye" */
		   { 3, 0, -1.0, {{0,0.5},{-1.0,0},{0,-0.5}}},
		   /* type 9b bar */
		   { 2, 1, 0.0, {{0,0.5},{0,-0.5}}},
		   /* type 10a two-prong fork */
		   { 4, 0, -1.0, {{0,0.5},{-1.0,0.5},{-1.0,-0.5},{0,-0.5}}},
		   /* type 10b backward two-prong fork */
		   { 4, 1, 0.0, {{-1.0,0.5,},{0,0.5},{0,-0.5},{-1.0,-0.5}}},
		};
static double		scalex, scaley;
static double		origx, origy;

void
geneps_option(opt, optarg)
char opt;
char *optarg;
{
	epsflag = True;
	gen_ps_eps_option(opt, optarg);
}

void
genps_option(opt, optarg)
char opt;
char *optarg;
{
	epsflag = False;
	gen_ps_eps_option(opt, optarg);
}

void
gen_ps_eps_option(opt, optarg)
char opt;
char *optarg;
{
	int i;

	switch (opt) {

	  case 'b':			/* border margin around figure */
		sscanf(optarg,"%d",&border_margin);
		break;

	  case 'c':			/* center figure */
		if (!epsflag) {
		    center = True;
		    centerspec = True;	/* user-specified */
		}
		break;

	  case 'e':			/* don't center ('e' means edge) figure */
		if (!epsflag) {
		    center = False;
		    centerspec = True;	/* user-specified */
		}
		break;

	  case 'f':			/* default font name */
		for ( i = 1; i <= MAX_PSFONT; i++ )
			if ( !strcmp(optarg, PSfontnames[i]) ) break;

		if ( i > MAX_PSFONT )
			fprintf(stderr,
			    "warning: non-standard font name %s\n", optarg);

	    	psfontnames[0] = psfontnames[1] = optarg;
	    	PSfontnames[0] = PSfontnames[1] = optarg;
	    	break;

	  case 'g':			/* background color */
		if (lookup_db_color(optarg,&background) >= 0) {
		    bgspec = True;
		} else {
		    fprintf(stderr,"Can't parse color '%s', ignoring background option\n",
				optarg);
		}
		break;

	  case 'M':			/* multi-page option */
		if (!epsflag) {
		    multi_page = True;
		    multispec = True;	/* user has overridden anything in file */
		}
		break;

	  case 'S':			/* turn off multi-page option */
		if (!epsflag) {
		    multi_page = False;
		    multispec = True;	/* user has overridden anything in file */
		}
		break;

      	  case 'm':			/* magnification (already parsed in main) */
		magspec = True;		/* user-specified */
		break;

	  /* don't do anything for language and font size (already parsed in main) */

      	  case 'L':			/* language */
	  case 's':			/* default font size */
		break;

	  case 'n':			/* name to put in the "Title:" spec */
		name = optarg;
		break;

      	  case 'l':			/* landscape mode */
		if (!epsflag) {
		    landscape = True;	/* override the figure file setting */
		    orientspec = True;	/* user-specified */
		}
		break;

      	  case 'p':			/* portrait mode */
		if (!epsflag) {
		    landscape = False;	/* override the figure file setting */
		    orientspec = True;	/* user-specified */
		}
		break;

	  case 'x':			/* x offset on page */
		if (!epsflag) {
		    xoff = atoi(optarg);
		}
		break;

	  case 'y':			/* y offset on page */
		if (!epsflag) {
		    yoff = atoi(optarg);
		}
		break;

	  case 'z':			/* papersize */
		if (!epsflag) {
		    (void) strcpy (papersize, optarg);
		    paperspec = True;	/* user-specified */
		}
		break;

	  default:
		put_msg(Err_badarg, opt, "ps");
		exit(1);
	}
}

void
genps_start(objects)
F_compound	*objects;
{
	char		host[256];
	struct passwd	*who;
	time_t		when;
	int		itmp;
	int		cliplx, cliply, clipux, clipuy;
	struct paperdef	*pd;
	float		bd;

	char		*libdir;
	char		filename[512], str[512];
	FILE		*fp;

	resolution = objects->nwcorner.x;
	coord_system = objects->nwcorner.y;
	scalex = scaley = mag * POINT_PER_INCH / (double)resolution;

	/* this seems to work around Solaris' cc optimizer bug */
	/* the problem was that llx had garbage in it - this "fixes" it */
	sprintf(host,"llx=%d\n",llx);

	/* convert to point unit */
	llx = (int)floor(llx * scalex);
	lly = (int)floor(lly * scaley);
	urx = (int)ceil(urx * scalex);
	ury = (int)ceil(ury * scaley);

	/* adjust for any border margin */
	bd = border_margin;

	llx -= bd;
	lly -= bd;
	urx += bd;
	ury += bd;

	/* convert ledger (deprecated) to tabloid */
	if (strcasecmp(papersize, "ledger") == 0)
		strcpy(papersize, "tabloid");
        for (pd = paperdef; pd->name != NULL; pd++)
	    if (strcasecmp (papersize, pd->name) == 0) {
		pagewidth = pd->width;
		pageheight = pd->height;
		strcpy(papersize,pd->name);	/* use the "nice" form */
		break;
	    }
	
	if (pagewidth < 0 || pageheight < 0) {
	    (void) fprintf (stderr, "Unknown paper size `%s'\n", papersize);
	    exit (1);
	}

	if (epsflag) {
	    /* eps, shift figure to 0,0 */
	    origx = -llx;
	    origy =  ury;
	} else {
	    /* postscript, do any orientation and/or centering */
	    if (landscape) {
		itmp = pageheight; pageheight = pagewidth; pagewidth = itmp;
		itmp = llx; llx = lly; lly = itmp;
		itmp = urx; urx = ury; ury = itmp;
	    }
	    if (center) {
		if (landscape) {
		    origx = (pageheight - urx - llx)/2.0;
		    origy = (pagewidth - ury - lly)/2.0;
		} else {
		    origx = (pagewidth - urx - llx)/2.0;
		    origy = (pageheight + ury + lly)/2.0;
		}
	    } else {
		origx = 0.0;
		origy = landscape ? 0.0 : pageheight;
	   }
	}

	/* finally, adjust by any offset the user wants */
	if (!epsflag) {
	    if (landscape) {
		origx += yoff;
		origy += xoff;
	    } else {
		origx += xoff;
		origy += yoff;
	    }
	}

	if (epsflag)
	    fprintf(tfp, "%%!PS-Adobe-2.0 EPSF-2.0\n");	/* Encapsulated PostScript */
	else
	    fprintf(tfp, "%%!PS-Adobe-2.0\n");		/* PostScript magic strings */

	who = getpwuid(getuid());
	if (gethostname(host, sizeof(host)) == -1)
	    (void)strcpy(host, "unknown-host!?!?");
	(void) time(&when);
	fprintf(tfp, "%%%%Title: %s\n",
		(name? name: ((from) ? from : "stdin")));
	fprintf(tfp, "%%%%Creator: %s Version %s Patchlevel %s\n",
		prog, VERSION, PATCHLEVEL);
	fprintf(tfp, "%%%%CreationDate: %s", ctime(&when));
	if (who)
	   fprintf(tfp, "%%%%For: %s@%s (%s)\n",
			who->pw_name, host, who->pw_gecos);

	/* calc initial clipping area to size of the bounding box (this is needed
		for later clipping by arrowheads */
	cliplx = cliply = 0;
	if (epsflag) {
	    clipux = urx-llx;
	    clipuy = ury-lly;
	    pages = 1;
	} else {
	    if (landscape) {
		clipux = pageheight;
		clipuy = pagewidth;
		pages = (urx/pageheight+1)*(ury/pagewidth+1);
		fprintf(tfp, "%%%%Orientation: Landscape\n");
	    } else {
		clipux = pagewidth;
		clipuy = pageheight;
		pages = (urx/pagewidth+1)*(ury/pageheight+1);
		fprintf(tfp, "%%%%Orientation: Portrait\n");
	    }
	    /* only print Pages if PostScript */
	    fprintf(tfp, "%%%%Pages: %d\n", pages );
	}
	fprintf(tfp, "%%%%BoundingBox: %d %d %d %d\n",
			cliplx, cliply, clipux, clipuy);

	/* only include a pagesize command if not EPS */
	if (!epsflag) {
	    fprintf(tfp, "%%%%BeginSetup\n");
	    fprintf(tfp, "%%%%IncludeFeature: *PageSize %s\n", papersize);
	    fprintf(tfp, "%%%%EndSetup\n");
	}

	/* put in the magnification for information purposes */
	fprintf(tfp, "%%%%Magnification: %.4f\n",metric? mag*76.2/80.0 : mag);

	fprintf(tfp, "%%%%EndComments\n");

	/* print any whole-figure comments prefixed with "%" */
	if (objects->comments) {
	    fprintf(tfp,"%%\n");
	    print_comments("% ",objects->comments, "");
	    fprintf(tfp,"%%\n");
	}

	/* insert PostScript codes to select paper size, if exist */
	libdir = getenv("FIG2DEV_LIBDIR");
#ifdef FIG2DEV_LIBDIR
	if (libdir == NULL)
	    libdir = FIG2DEV_LIBDIR;
#endif
	if (libdir != NULL) {
	  sprintf(filename, "%s/%s.ps", libdir, papersize);
	  /* get filename like "/usr/local/lib/fig2dev/A3.ps" */
	  fp = fopen(filename, "r");
	  if (fp != NULL) {
	    while (fgets(str, sizeof(str), fp)) fputs(str, tfp);
	    fclose(fp);
	  }
	}

	if (pats_used)
		fprintf(tfp,"/MyAppDict 100 dict dup begin def\n");
	fprintf(tfp, "%s", BEGIN_PROLOG1);
	/* define the standard colors */
	genps_std_colors();
	/* define the user colors */
	genps_usr_colors();
	fprintf(tfp, "\nend\n");

	/* must specify translation/rotation before definition of fill patterns */
	fprintf(tfp, "save\n");
 
	/* now make the clipping path for the BoundingBox */
	fprintf(tfp, "newpath %d %d moveto %d %d lineto %d %d lineto %d %d lineto closepath clip newpath\n",
		cliplx,clipuy, cliplx,cliply, clipux,cliply, clipux,clipuy);

	/* fill the Background now if specified */
 	if (bgspec) {
 	    fprintf(tfp, "%% Fill background color\n");
 	    fprintf(tfp, "%d %d moveto %d %d lineto ",
					cliplx, cliply, clipux, cliply);
 	    fprintf(tfp, "%d %d lineto %d %d lineto\n",
					clipux, clipuy, cliplx, clipuy);
 	    fprintf(tfp, "closepath %.2f %.2f %.2f setrgbcolor fill\n\n",
 		    background.red/65536.0,
 		    background.green/65536.0,
 		    background.blue/65536.0);
 	}

	fprintf(tfp, "%.1f %.1f translate\n", origx, origy);
	/* also flip y if necessary, but *only* if to PostScript and not EPS */
	if (landscape && !epsflag) {
	    fprintf(tfp, " 90 rotate\n");
	}
	if (coord_system == 2) {
	    fprintf(tfp, "1 -1 scale\n");
	}
	if (pats_used) {
	    int i;
	    fprintf(tfp, ".9 .9 scale %% to make patterns same scale as in xfig\n");
	    fprintf(tfp, "\n%s%s%s", FILL_PROLOG1,FILL_PROLOG2,FILL_PROLOG3);
	    fprintf(tfp, "\n%s%s%s", FILL_PROLOG4,FILL_PROLOG5,FILL_PROLOG6);
	    fprintf(tfp, "\n%s%s%s", FILL_PROLOG7,FILL_PROLOG8,FILL_PROLOG9);
	    fprintf(tfp, "\n%s%s",   FILL_PROLOG10,FILL_PROLOG11);
	    /* only define the patterns that are used */
	    for (i=0; i<NUMPATTERNS; i++)
		if (pattern_used[i])
			fprintf(tfp, "\n%s", fill_def[i]);
	    fprintf(tfp, "1.1111 1.1111 scale %%restore scale\n");
	}
	fprintf(tfp, "\n%s", BEGIN_PROLOG2);
	if (iso_text_exist(objects)) {
	   fprintf(tfp, "%s%s%s", SPECIAL_CHAR_1,SPECIAL_CHAR_2,SPECIAL_CHAR_3);
	   encode_all_fonts(objects);
	}
	if (ellipse_exist(objects))
		fprintf(tfp, "%s\n", ELLIPSE_PS);
	if (approx_spline_exist(objects))
		fprintf(tfp, "%s\n", SPLINE_PS);
#ifdef I18N
	if (support_i18n && iso_text_exist(objects)) {
	    char *libdir, *locale;
	    char filename[512], str[512];
	    FILE *fp;
	    libdir = getenv("FIG2DEV_LIBDIR");
	    if (libdir == NULL)
		libdir = FIG2DEV_LIBDIR;
	    locale = getenv("LANG");
	    if (locale == NULL) {
		fprintf(stderr, "fig2dev: LANG not defined; assuming C locale\n");
		locale = "C";
	    }
	    sprintf(filename, "%s/%s.ps", libdir, locale);
	    /* get filename like ``/usr/local/lib/fig2dev/japanese.ps'' */
	    fp = fopen(filename, "r");
	    if (fp == NULL) {
		fprintf(stderr, "fig2dev: can't open file: %s\n", filename);
	    } else {
		while (fgets(str, sizeof(str), fp)) {
		    if (strstr(str, "CompositeRoman")) enable_composite_font = True;
		    fputs(str, tfp);
		}
	        fclose(fp);
	    }
	}
#endif /* I18N */
	
	fprintf(tfp, "%s\n", END_PROLOG);

	fprintf(tfp, "$F2psBegin\n");

	fprintf(tfp, "%%%%Page: 1 1\n");
	fprintf(tfp, "10 setmiterlimit\n");	/* make like X server (11 degrees) */

 	if ( multi_page ) {
	    fprintf(tfp, "initmatrix\n");
	} else {
	    fprintf(tfp, " %.5f %.5f sc\n", scalex, scaley );
	    if (!epsflag)
		fprintf(tfp,"%%%%Page: 1 1\n");
	}
}

int
genps_end()
{
    double dx,dy;
    int i, page;
    int h,w;

    if (multi_page) {
       page = 1;
       h = (landscape? pagewidth: pageheight);
       w = (landscape? pageheight: pagewidth);
       for (dy=0; dy < (ury-h*0.1); dy += h*0.9) {
	 for (dx=0; dx < (urx-w*0.1); dx += w*0.9) {
	    fprintf(tfp, "%%%%Page: %d %d\n",page,page);
	    fprintf(tfp, "gs\n");
	    fprintf(tfp,"%.1f %.1f tr", 
		-dx, (landscape? -dy:(dy+h*0.9)));
	    if (landscape) {
	       fprintf(tfp, " 90 rot");
	    }
	    if (coord_system == 2) {
		fprintf(tfp, " 1 -1 sc\n");
	    }
	    fprintf(tfp, " %.3f %.3f sc\n", scalex, scaley);
	    for (i=0; i<no_obj; i++) {
	       fprintf(tfp, "o%d ", i);
	       if (!(i%20)) 
		  fprintf(tfp, "\n");
	    }
	    fprintf(tfp, "gr\n");
	    fprintf(tfp, "showpage\n");
	    page++;
	 }
       }
    } 
    fprintf(tfp, "$F2psEnd\n");
    fprintf(tfp, "rs\n");
    if (pats_used)
	fprintf(tfp, "end\n");		/* close off MyAppDict */
    /* add showpage if requested */
    if (!multi_page && !epsflag)
	fprintf(tfp, "showpage\n");

    /* all ok */
    return 0;
}

static
set_style(s, v)
int	s;
double	v;
{
	v /= 80.0 / (double)resolution;
	if (s == DASH_LINE) {
	    if (v > 0.0) fprintf(tfp, " [%d] 0 sd\n", round(v));
	    }
	else if (s == DOTTED_LINE) {
	    if (v > 0.0) fprintf(tfp, " [%d %d] %d sd\n", 
		round(resolution/80.0), round(v), round(v));
	    }
	else if (s == DASH_DOT_LINE) {
	    if (v > 0.0) fprintf(tfp, " [%d %d %d %d] 0 sd\n", 
		round(v), round(v*0.5),
		round(resolution/80.0), round(v*0.5));
	    }
	else if (s == DASH_2_DOTS_LINE) {
	    if (v > 0.0) fprintf(tfp, " [%d %d %d %d %d %d] 0 sd\n", 
		round(v), round(v*0.45),
		round(resolution/80.0), round(v*0.333),
		round(resolution/80.0), round(v*0.45));
	    }
	else if (s == DASH_3_DOTS_LINE) {
	    if (v > 0.0) fprintf(tfp, 
                " [%d %d %d %d %d %d %d %d ] 0 sd\n", 
		round(v), round(v*0.4),
		round(resolution/80.0), round(v*0.3),
		round(resolution/80.0), round(v*0.3),
		round(resolution/80.0), round(v*0.4));
	    }
	}

static
reset_style(s, v)
int	s;
double	v;
{
	if (s == DASH_LINE) {
	    if (v > 0.0) fprintf(tfp, " [] 0 sd");
	    }
	else if (s == DOTTED_LINE) {
	    if (v > 0.0) fprintf(tfp, " [] 0 sd");
	    }
	else if (s == DASH_DOT_LINE || s == DASH_2_DOTS_LINE ||
                 s == DASH_3_DOTS_LINE) {
	    if (v > 0.0) fprintf(tfp, " [] 0 sd");
	    }
	fprintf(tfp, "\n");
	}

static
set_linejoin(j)
int	j;
{
	if (j != cur_joinstyle) {
	    cur_joinstyle = j;
	    fprintf(tfp, "%d slj\n", cur_joinstyle);
	    }
	}

static
set_linecap(j)
int	j;
{
	if (j != cur_capstyle) {
	    cur_capstyle = j;
	    fprintf(tfp, "%d slc\n", cur_capstyle);
	    }
	}

static
set_linewidth(w)
double	w;
{
	if (w != cur_thickness) {
	    cur_thickness = w;
	    fprintf(tfp, "%.3f slw\n",
		    cur_thickness <= THICK_SCALE ? 	/* make lines a little thinner */
				0.5* cur_thickness :
				cur_thickness - THICK_SCALE);
	    }
	}

FILE	*open_picfile();
void	close_picfile();
int	filtype;
extern	int	read_gif();
extern	int	read_pcx();
extern	int	read_eps();
extern	int	read_pdf();
extern	int	read_ppm();
extern	int	read_tif();
extern	int	read_xbm();
#ifdef USE_JPEG
extern	int	read_jpg();
#endif
#ifdef USE_XPM
extern	int	read_xpm();
#endif

/* headers for various image files */

static	 struct hdr {
	    char	*type;
	    char	*bytes;
	    int		 nbytes;
	    int		(*readfunc)();
	    Boolean	pipeok;
	}
	headers[]= {    {"GIF", "GIF",		    3, read_gif,	True},
#ifdef V4_0
			{"FIG", "#FIG",		    4, read_figure,	True},
#endif /* V4_0 */
			{"PCX", "\012\005\001",	    3, read_pcx,	True},
			{"EPS", "%!",		    2, read_eps,	True},
			{"PDF", "%PDF",		    4, read_pdf,	True},
			{"PPM", "P3",		    2, read_ppm,	True},
			{"PPM", "P6",		    2, read_ppm,	True},
			{"TIFF", "DD*\000",	    4, read_tif,	False},
			{"TIFF", "MM\000*",	    4, read_tif,	False},
			{"XBM", "#define",	    7, read_xbm,	True},
#ifdef USE_JPEG
			{"JPEG", "\377\330\377\340", 4, read_jpg,	True},
#endif
#ifdef USE_XPM
			{"XPM", "/* XPM */",	    9, read_xpm,	False},
#endif
			};

#define NUMHEADERS sizeof(headers)/sizeof(headers[0])


void
genps_line(l)
F_line	*l;
{
	F_point		*p, *q;
	/* JNT */
	int		radius;
	int		i;
	FILE		*picf;
	char		buf[512], realname[PATH_MAX];
	char		*cp;
	int		xmin,xmax,ymin,ymax;
	int		pic_w, pic_h;
	Boolean		namedcol;
	float		hf_wid;
	
	if (multi_page)
	   fprintf(tfp, "/o%d {", no_obj++);

	/* print any comments prefixed with "%" */
	print_comments("% ",l->comments, "");

	fprintf(tfp, "%% Polyline\n");
	if (l->type != T_PIC_BOX) {  /* pic object has no line thickness */
		set_linejoin(l->join_style);
		set_linecap(l->cap_style);
		set_linewidth((double)l->thickness);
	}
	p = l->points;
	q = p->next;
	if (q == NULL) { /* A single point line */
	    if (l->cap_style > 0)
		hf_wid = 1.0;
	    else if (l->thickness <= THICK_SCALE)
		hf_wid = l->thickness/4.0;
	    else
		hf_wid = (l->thickness-THICK_SCALE)/2.0;
	    fprintf(tfp, "n %d %d m %d %d l gs col%d s gr\n",
			round(p->x-hf_wid), p->y, round(p->x+hf_wid), p->y, l->pen_color);
	    if (multi_page)
	       fprintf(tfp, "} bind def\n");
	    return;
	}
	if (l->type != T_PIC_BOX) {
	    set_style(l->style, l->style_val);
	}

	xmin = xmax = p->x;
	ymin = ymax = p->y;
	while (p->next != NULL) {	/* find lower left and upper right corners */
	    p=p->next;
	    if (xmin > p->x)
		xmin = p->x;
	    else if (xmax < p->x)
		xmax = p->x;
	    if (ymin > p->y)
		ymin = p->y;
	    else if (ymax < p->y)
		ymax = p->y;
	    }

	if (l->type == T_ARC_BOX) {
	  /* ARC BOX */
	    radius = l->radius;		/* radius of the corner */
	    /* limit the radius to the smaller of the two sides or postscript crashes */
	    /* from T.Sato */
	    if ((xmax - xmin) / 2 < radius) 
		radius = (xmax - xmin) / 2;
	    if ((ymax - ymin) / 2 < radius) 
		radius = (ymax - ymin) / 2;
	    fprintf(tfp, "n %d %d m",xmin+radius, ymin);
	    fprintf(tfp, " %d %d %d %d %d arcto 4 {pop} repeat\n",
				xmin, ymin, xmin, ymax-radius, radius);
	    fprintf(tfp, "  %d %d %d %d %d arcto 4 {pop} repeat\n", /* arc through bl to br */
				xmin, ymax, xmax-radius, ymax, radius);
	    fprintf(tfp, "  %d %d %d %d %d arcto 4 {pop} repeat\n", /* arc through br to tr */
				xmax, ymax, xmax, ymin+radius, radius);
	    fprintf(tfp, "  %d %d %d %d %d arcto 4 {pop} repeat\n", /* arc through tr to tl */
				xmax, ymin, xmin+radius, ymin, radius);
	} else if (l->type == T_PIC_BOX) {  /* imported picture */
	  /* PICTURE OBJECT */
		int             dx, dy, rotation;
		int		llx, lly, urx, ury;
		int		i, j;
		Boolean		found;
		int		c;
		double          fllx, flly, furx, fury;

		dx = l->points->next->next->x - l->points->x;
		dy = l->points->next->next->y - l->points->y;
		rotation = 0;
		if (dx < 0 && dy < 0)
			   rotation = 180;
		else if (dx < 0 && dy >= 0)
			   rotation = 90;
		else if (dy < 0 && dx >= 0)
			   rotation = 270;

		fprintf(tfp, "%%\n");

		/* open the file and read a few bytes of the header to see what it is */
		if ((picf=open_picfile(l->pic->file, &filtype, True, realname)) == NULL) {
			fprintf(stderr,"No such picture file: %s",l->pic->file);
			return;
		}

		for (i=0; i<15; i++) {
		    if ((c=getc(picf))==EOF)
		    break;
		    buf[i]=(char) c;
		}
		close_picfile(picf,filtype);

		/* now find which header it is */
		for (i=0; i<NUMHEADERS; i++) {
		    found = True;
		    for (j=headers[i].nbytes-1; j>=0; j--)
		    if (buf[j] != headers[i].bytes[j]) {
			found = False;
			break;
		    }
		    if (found)
		    break;
		}
		if (found) {
		    /* open it again (it may be a pipe so we can't just rewind) */
		    picf=open_picfile(l->pic->file, &filtype, headers[i].pipeok, realname);

		    if (headers[i].pipeok) {
			if (((*headers[i].readfunc)(picf,filtype,l->pic,&llx,&lly)) == 0) {
			    fprintf(stderr,"%s: Bad %s format",l->pic->file, headers[i].type);
			    close_picfile(picf,filtype);
			    return;
			}
		    } else {
			/* routines that can't take a pipe (e.g. xpm) get the real filename */
			if (((*headers[i].readfunc)(realname,filtype,l->pic,&llx,&lly)) == 0) {
			    fprintf(stderr,"%s: Bad %s format",l->pic->file, headers[i].type);
			    close_picfile(picf,filtype);
			    return;
			}
		    }
		    /* Successful read */
		    close_picfile(picf,filtype);
		} else {
		    /* none of the above */
		    fprintf(stderr,"%s: Unknown image format\n",l->pic->file);
		    return;
		}
		urx = l->pic->bit_size.x+llx;	/* calc upper-right from size and lower-left */
		ury = l->pic->bit_size.y+lly;

		fprintf(tfp, "n gs\n");
		if (((rotation == 90 || rotation == 270) && !l->pic->flipped) ||
		    (rotation != 90 && rotation != 270 && l->pic->flipped)) {
			pic_h = urx - llx;
			pic_w = ury - lly;
		} else {
			pic_w = urx - llx;
			pic_h = ury - lly;
		}

		/* translate the pic stuff to the right spot on the page */
		fprintf(tfp, "%d %d tr\n", xmin, ymin);

		/* scale the pic stuff to fit into the bounding box */
		/* Note: the origin for fig is in the upper-right corner;
		 *       for postscript its in the lower right hand corner.
		 *       To fix it, we use a "negative"-y scale factor, then
		 *       translate the image up on the page */

		fprintf(tfp, "%f %f sc\n",
			fabs((double)(xmax-xmin)/pic_w), -1.0*(double)(ymax-ymin)/pic_h);

		/* flip the pic stuff */
		/* always translate it back so that the lower-left corner is at the origin */

		/* note: fig measures rotation clockwise; postscript is counter-clockwise */
		/* always translate it back so that the lower-left corner is at the origin */
		switch (rotation) {
		   case 0:
			if (l->pic->flipped) {
				fprintf(tfp, "%d 0 tr\n", pic_w);
				fprintf(tfp, "%d rot\n", 270);
				fprintf(tfp, "1 -1 sc\n");
			} else {
				fprintf(tfp, "0 %d tr\n", -pic_h);
			}
			break;
		   case 90:
			if (l->pic->flipped) {
				fprintf(tfp, "%d %d tr\n", pic_w, -pic_h);
				fprintf(tfp, "-1 1 sc\n");
			} else {
				fprintf(tfp, "%d rot\n", 270);
			}
			break;
		   case 180:
			if (l->pic->flipped) {
				fprintf(tfp, "0 %d tr\n", -pic_h);
				fprintf(tfp, "%d rot\n", 270);
				fprintf(tfp, "-1 1 sc\n");
			} else {
				fprintf(tfp, "%d 0 tr\n", pic_w);
				fprintf(tfp, "%d rot\n", 180);
			}
			break;
		   case 270:
			if (l->pic->flipped) {
				fprintf(tfp, "1 -1 sc\n");
			} else {
				fprintf(tfp, "%d %d tr\n", pic_w, -pic_h);
				fprintf(tfp, "%d rot\n", 90);
			}
			break;
		}

		/* translate the pic stuff so that the lower-left corner is at the origin */
		fprintf(tfp, "%d %d tr\n", -llx, -lly);
		/* save vm so pic file won't change anything */
		fprintf(tfp, "sa\n");

		/* if PIC object is EPS file, set up clipping rectangle to BB
		 * and prepare to clean up stacks and dicts of included EPS file
		 */
		if (l->pic->subtype == P_EPS) {
		    fprintf(tfp, "n %d %d m %d %d l %d %d l %d %d l cp clip n\n",
			llx,lly, urx,lly, urx,ury, llx,ury);
		    fprintf(tfp, "countdictstack\n");
		    fprintf(tfp, "mark\n");
		}

		/* and undefine showpage */
		fprintf(tfp, "/showpage {} def\n");

		/* XBM file */
		if (l->pic->subtype == P_XBM) {
			int		 i,j;
			unsigned char	*bit;
			int		 cwid;

			fprintf(tfp, "col%d\n ", l->pen_color);
			fprintf(tfp, "%% Bitmap image follows:\n");
			/* scale for size in bits */
			fprintf(tfp, "%d %d sc\n", urx, ury);
			fprintf(tfp, "/pix %d string def\n", (int)((urx+7)/8));
			/* width, height and paint 0 bits */
			fprintf(tfp, "%d %d false\n", urx, ury);
			/* transformation matrix */
			fprintf(tfp, "[%d 0 0 %d 0 %d]\n", urx, -ury, ury);
			/* function for reading bits */
			fprintf(tfp, "{currentfile pix readhexstring pop}\n");
			/* use imagemask to draw in color */
			fprintf(tfp, "imagemask\n");
			bit = l->pic->bitmap;
			cwid = 0;
			for (i=0; i<ury; i++) {				/* for each row */
			    for (j=0; j<(int)((urx+7)/8); j++) {	/* for each byte */
				fprintf(tfp,"%02x", (unsigned char) ~(*bit++));
				cwid+=2;
				if (cwid >= 80) {
				    fprintf(tfp,"\n");
				    cwid=0;
				}
			    }
			    fprintf(tfp,"\n");
			}

#ifdef USE_XPM
		/* XPM file */
		} else if (l->pic->subtype == P_XPM) {
			int	  i, wid, ht;
			XpmColor *coltabl;
			char	 *c;
			int	  r,g,b;
			unsigned char *cdata, *cp;
			unsigned int  *dp;

			/* start with width and height */
			wid = l->pic->xpmimage.width;
			ht = l->pic->xpmimage.height;
			fprintf(tfp, "%% Pixmap image follows:\n");
			/* scale for size in bits */
			fprintf(tfp, "%d %d sc\n", urx, ury);
			/* modify colortable entries to make consistent */
			coltabl = l->pic->xpmimage.colorTable;
			/* convert the colors to rgb constituents */
			convert_xpm_colors(l->pic->cmap,coltabl,l->pic->xpmimage.ncolors);
			/* and convert the integer data to unsigned char */
			dp = l->pic->xpmimage.data;
			if ((cdata = (unsigned char *)
			     malloc(wid*ht*sizeof(unsigned char))) == NULL) {
				fprintf(stderr,"can't allocate space for XPM image\n");
				return;
			}
			cp = cdata;
			for (i=0; i<wid*ht; i++)
			    *cp++ = (unsigned char) *dp++;
				
			/* now write out the image data in a compressed form */
			(void) PSencode(tfp, wid, ht, l->pic->xpmimage.ncolors,
				l->pic->cmap[0], l->pic->cmap[1], l->pic->cmap[2], 
				cdata);
			/* and free up the space */
			free(cdata);
			XpmFreeXpmImage(&l->pic->xpmimage);
#endif /* USE_XPM */

		/* GIF, PCX, or JPEG file */
		} else if (l->pic->subtype == P_GIF || l->pic->subtype == P_PCX || 
				l->pic->subtype == P_JPEG) {
			int		 wid, ht;

			/* start with width and height */
			wid = l->pic->bit_size.x;
			ht = l->pic->bit_size.y;
			if (l->pic->subtype == P_GIF)
			    fprintf(tfp, "%% GIF image follows:\n");
			else if (l->pic->subtype == P_PCX)
			    fprintf(tfp, "%% PCX image follows:\n");
			else
			    fprintf(tfp, "%% JPEG image follows:\n");
			/* scale for size in bits */
			fprintf(tfp, "%d %d sc\n", urx, ury);
			/* now write out the image data in a compressed form */
			(void) PSencode(tfp, wid, ht, l->pic->numcols,
				l->pic->cmap[0], l->pic->cmap[1], l->pic->cmap[2], 
				l->pic->bitmap);

		/* EPS file */
		} else if (l->pic->subtype == P_EPS) {
		    int i;
		    fprintf(tfp, "%% EPS file follows:\n");
		    if ((picf=open_picfile(l->pic->file, &filtype, True, realname)) == NULL) {
			fprintf(stderr, "Unable to open EPS file '%s': error: %s (%d)\n",
				l->pic->file, sys_errlist[errno],errno);
			fprintf(tfp, "gr\n");
			return;
		    }
		    /* use read/write() calls in case of binary data! */
		    /* but flush buffer first */
		    fflush(tfp);
		    while ((i = read(fileno(picf),buf,sizeof(buf))) > 0) {
			write(fileno(tfp),buf,i);
		    }
		    close_picfile(picf,filtype);
		}

		/* if PIC object is EPS file, clean up stacks and dicts
		 * before 'restore'ing vm
		 */
		if (l->pic->subtype == P_EPS) {
		    fprintf(tfp, "\ncleartomark\n");
		    fprintf(tfp, "countdictstack exch sub { end } repeat\n");
		}

		/* restore vm and gsave */
		fprintf(tfp, "restore grestore\n");
		fprintf(tfp, "%%\n");
		fprintf(tfp, "%% End Imported PIC File: %s\n", l->pic->file);
		if (l->pic->subtype == P_EPS)
		    fprintf(tfp, "%%%%EndDocument\n");
		fprintf(tfp, "%%\n");
	} else {
	  /* POLYLINE */
		p = l->points;
		q = p->next;
		/* first point */
		fpntx1 = p->x;
		fpnty1 = p->y;
		/* second point */
		fpntx2 = q->x;
		fpnty2 = q->y;
		/* go through the points to get the last two */
		while (q->next != NULL) {
		    p = q;
		    q = q->next;
		}
		/* next to last point */
		lpntx2 = p->x;
		lpnty2 = p->y;
		/* last point */
		lpntx1 = q->x;
		lpnty1 = q->y;
		/* set clipping for any arrowheads */
		if (l->for_arrow || l->back_arrow) {
		    fprintf(tfp, "gs ");
		    clip_arrows(l, O_POLYLINE);
		}

		/* now output the points */
		p = l->points;
		q = p->next;
		fprintf(tfp, "n %d %d m", p->x, p->y);
		i=1;
		while (q->next != NULL) {
		    p = q;
		    q = q->next;
		    fprintf(tfp, " %d %d l", p->x, p->y);
 	    	    if (i%5 == 0)
			fprintf(tfp, "\n");
		    i++;
		}
		fprintf(tfp, "\n");
	}

	/* now fill it, draw the line and/or draw arrow heads */
	if (l->type != T_PIC_BOX) {	/* make sure it isn't a picture object */
		if (l->type == T_POLYLINE) {
		    fprintf(tfp, " %d %d l ", q->x, q->y);
		    if (fpntx1==lpntx1 && fpnty1==lpnty1)
			fprintf(tfp, " cp ");	/* endpoints are coincident, close path 
							so that line join is used */
		} else {
		    fprintf(tfp, " cp ");	/* polygon, close path */
		}
		/* fill it if there is a fill style */
		if (l->fill_style != UNFILLED)
		    fill_area(l->fill_style, l->pen_color, l->fill_color, xmin, ymin);
		/* stroke if there is a line thickness */
		if (l->thickness > 0)
		     fprintf(tfp, "gs col%d s gr ", l->pen_color);

		/* reset clipping */
		if (l->type == T_POLYLINE && ((l->for_arrow || l->back_arrow)))
		    fprintf(tfp,"gr\n");
		reset_style(l->style, l->style_val);

		if (l->back_arrow && l->thickness > 0)
		    draw_arrow(l, l->back_arrow, bpoints, nbpoints, l->pen_color);
		if (l->for_arrow && l->thickness > 0)
		    draw_arrow(l, l->for_arrow, fpoints, nfpoints, l->pen_color);
	}
	if (multi_page)
	   fprintf(tfp, "} bind def\n");
}

void 
genps_spline(s)
F_spline	*s;
{
	if (multi_page)
	   fprintf(tfp, "/o%d {", no_obj++);

	/* print any comments prefixed with "%" */
	print_comments("% ",s->comments, "");

	if (closed_spline(s)) {
	    if (s->style == DOTTED_LINE)
		set_linecap(1);		/* round dots for dotted line */
	} else {
	    set_linecap(s->cap_style);	/* open splines can explicitely set capstyle */
	}
	/* set the line thickness */
	set_linewidth((double)s->thickness);
	if (int_spline(s))
	    genps_itp_spline(s);
	else
	    genps_ctl_spline(s);
	if (multi_page)
	   fprintf(tfp, "} bind def\n");
	}

genps_itp_spline(s)
F_spline	*s;
{
	F_point		*p, *q;
	F_control	*a, *b, *ar;
	int		 xmin, ymin;

	fprintf(tfp, "%% Interp Spline\n");
	a = ar = s->controls;

	a = s->controls;
	p = s->points;
	/* first point */
	fpntx1 = p->x;
	fpnty1 = p->y;
	/* second point */
	fpntx2 = round(a->rx);
	fpnty2 = round(a->ry);
	/* go through the points to find the last two */
	for (q = p->next; q != NULL; p = q, q = q->next) {
	    b = a->next;
	    a = b;
	}
	/* next to last point */
	lpntx2 = round(b->lx);
	lpnty2 = round(b->ly);
	/* last point */
	lpntx1 = p->x;
	lpnty1 = p->y;
	/* set clipping for any arrowheads */
	fprintf(tfp, "gs ");
	if (s->for_arrow || s->back_arrow)
	    clip_arrows(s, O_SPLINE);

	a = s->controls;
	p = s->points;
	set_style(s->style, s->style_val);
	fprintf(tfp, "n %d %d m\n", p->x, p->y);
	xmin = 999999;
	ymin = 999999;
	for (q = p->next; q != NULL; p = q, q = q->next) {
	    xmin = min(xmin, p->x);
	    ymin = min(ymin, p->y);
	    b = a->next;
	    fprintf(tfp, "\t%.1f %.1f %.1f %.1f %d %d curveto\n",
			a->rx, a->ry, b->lx, b->ly, q->x, q->y);
	    a = b;
	    }
	if (closed_spline(s)) fprintf(tfp, " cp ");
	if (s->fill_style != UNFILLED)
	    fill_area(s->fill_style, s->pen_color, s->fill_color, xmin, ymin);
	if (s->thickness > 0)
	    fprintf(tfp, " gs col%d s gr\n", s->pen_color);
	/* reset clipping */
	fprintf(tfp," gr\n");
	reset_style(s->style, s->style_val);

	/* draw arrowheads after spline for open arrow */
	if (s->back_arrow && s->thickness > 0)
	    draw_arrow(s, s->back_arrow, bpoints, nbpoints, s->pen_color);

	if (s->for_arrow && s->thickness > 0)
	    draw_arrow(s, s->for_arrow, fpoints, nfpoints, s->pen_color);
	}

genps_ctl_spline(s)
F_spline	*s;
{
	double		a, b, c, d, x1, y1, x2, y2, x3, y3;
	F_point		*p, *q;
	double		xx,yy;
	int		xmin, ymin;
	Boolean		first = True;

	if (closed_spline(s))
	    fprintf(tfp, "%% Closed spline\n");
	else
	    fprintf(tfp, "%% Open spline\n");

	p = s->points;
	x1 = p->x;
	y1 = p->y;
	p = p->next;
	c = p->x;
	d = p->y;
	x3 = a = (x1 + c) / 2;
	y3 = b = (y1 + d) / 2;

	/* first point */
	fpntx1 = round(x1);
	fpnty1 = round(y1);
	/* second point */
	fpntx2 = round(x3);
	fpnty2 = round(y3);

	/* in case there are only two points in this spline */
	x2=x1; y2=y1;
	/* go through the points to find the last two */
	for (q = p->next; q != NULL; p = q, q = q->next) {
	    x1 = x3;
	    y1 = y3;
	    x2 = c;
	    y2 = d;
	    c = q->x;
	    d = q->y;
	    x3 = (x2 + c) / 2;
	    y3 = (y2 + d) / 2;
	}
	/* next to last point */
	lpntx2 = round(x2);
	lpnty2 = round(y2);
	/* last point */
	lpntx1 = round(c);
	lpnty1 = round(d);
	/* set clipping for any arrowheads */
	fprintf(tfp, "gs ");
	if (s->for_arrow || s->back_arrow)
	    clip_arrows(s, O_SPLINE);

	/* now output the points */
	set_style(s->style, s->style_val);
	xmin = 999999;
	ymin = 999999;

	p = s->points;
	x1 = p->x;
	y1 = p->y;
	p = p->next;
	c = p->x;
	d = p->y;
	x3 = a = (x1 + c) / 2;
	y3 = b = (y1 + d) / 2;
	/* in case there are only two points in this spline */
	x2=x1; y2=y1;
	if (closed_spline(s))
	    fprintf(tfp, "n %.1f %.1f m\n", a, b);
	else
	    fprintf(tfp, "n %.1f %.1f m %.1f %.1f l\n", x1, y1, x3, y3);
	
	for (q = p->next; q != NULL; p = q, q = q->next) {
	    xmin = min(xmin, p->x);
	    ymin = min(ymin, p->y);
	    x1 = x3;
	    y1 = y3;
	    x2 = c;
	    y2 = d;
	    c = q->x;
	    d = q->y;
	    x3 = (x2 + c) / 2;
	    y3 = (y2 + d) / 2;
	    fprintf(tfp, "\t%.1f %.1f %.1f %.1f %.1f %.1f DrawSplineSection\n",
			x1, y1, x2, y2, x3, y3);
	}
	/*
	* At this point, (x2,y2) and (c,d) are the position of the
	* next-to-last and last point respectively, in the point list
	*/
	if (closed_spline(s)) {
	    fprintf(tfp, "\t%.1f %.1f %.1f %.1f %.1f %.1f DrawSplineSection closepath ",
			x3, y3, c, d, a, b);
	} else {
	    fprintf(tfp, "\t%.1f %.1f l ", c, d);
	}
	if (s->fill_style != UNFILLED)
	    fill_area(s->fill_style, s->pen_color, s->fill_color, xmin, ymin);
	if (s->thickness > 0)
	    fprintf(tfp, " gs col%d s gr\n", s->pen_color);
	/* reset clipping */
	fprintf(tfp," gr\n");
	reset_style(s->style, s->style_val);

	/* draw arrowheads after spline */
	if (s->back_arrow && s->thickness > 0)
	    draw_arrow(s, s->back_arrow, bpoints, nbpoints, s->pen_color);
	if (s->for_arrow && s->thickness > 0)
	    draw_arrow(s, s->for_arrow, fpoints, nfpoints, s->pen_color);
	}

void
genps_arc(a)
F_arc	*a;
{
	double		angle1, angle2, dx, dy, radius, x, y;
	double		cx, cy, sx, sy, ex, ey;
	int		direction;

	if (multi_page)
	   fprintf(tfp, "/o%d {", no_obj++);

	/* print any comments prefixed with "%" */
	print_comments("% ",a->comments, "");

	fprintf(tfp, "%% Arc\n");

	cx = a->center.x; cy = a->center.y;
	sx = a->point[0].x; sy = a->point[0].y;
	ex = a->point[2].x; ey = a->point[2].y;

	if (coord_system != 2)
	    direction = !a->direction;
	else
	    direction = a->direction;
	set_linewidth((double)a->thickness);
	set_linecap(a->cap_style);
	dx = cx - sx;
	dy = cy - sy;
	radius = sqrt(dx*dx+dy*dy);
	if (cx==sx)
	    angle1 = (sy-cy > 0? 90.0: -90.0);
	else
	    angle1 = atan2(sy-cy, sx-cx) * 180.0 / M_PI;
	if (cx==ex)
	    angle2 = (ey-cy > 0? 90.0: -90.0);
	else
	    angle2 = atan2(ey-cy, ex-cx) * 180.0 / M_PI;

	if ((a->type == T_OPEN_ARC) && (a->thickness != 0) && (a->back_arrow || a->for_arrow)) {
	    /* set clipping for any arrowheads */
	    fprintf(tfp, "gs ");
	    if (a->for_arrow || a->back_arrow)
		clip_arrows(a, O_ARC);
	}

	set_style(a->style, a->style_val);

	/* draw the arc now */
	/* direction = 1 -> Counterclockwise */
	fprintf(tfp, "n %.1f %.1f %.1f %.1f %.1f %s\n",
		cx, cy, radius, angle1, angle2,
		((direction == 1) ? "arcn" : "arc"));

	if (a->type == T_PIE_WEDGE_ARC)
		fprintf(tfp,"%.1f %.1f l %.1f %.1f l ",cx,cy,sx,sy);

	/******	The upper-left values (dx, dy) aren't really correct so	  ******/
	/******	the fill pattern alignment between a filled arc and other ******/
	/******	filled objects will not be correct			  ******/
	if (a->fill_style != UNFILLED)
	    fill_area(a->fill_style, a->pen_color, a->fill_color, (int)dx, (int)dy);
	if (a->thickness > 0)
	    fprintf(tfp, "gs col%d s gr\n", a->pen_color);

	if ((a->type == T_OPEN_ARC) && (a->thickness != 0) && (a->back_arrow || a->for_arrow)) {
	    /* reset clipping */
	    fprintf(tfp," gr\n");
	}
	reset_style(a->style, a->style_val);

	/* now draw the arrowheads, if any */
	if (a->type == T_OPEN_ARC) {
	    if (a->for_arrow && a->thickness > 0)
		draw_arrow(a, a->for_arrow, fpoints, nfpoints, a->pen_color);
	    if (a->back_arrow && a->thickness > 0)
		draw_arrow(a, a->back_arrow, bpoints, nbpoints, a->pen_color);
	}
	if (multi_page)
	   fprintf(tfp, "} bind def\n");
	}

void
genps_ellipse(e)
F_ellipse	*e;
{
	if (multi_page)
	   fprintf(tfp, "/o%d {", no_obj++);

	/* print any comments prefixed with "%" */
	print_comments("% ",e->comments, "");

	set_linewidth((double)e->thickness);
	set_style(e->style, e->style_val);
	if (e->style == DOTTED_LINE)
	    set_linecap(1);	/* round dots */
	if (e->angle == 0)
	{
	    fprintf(tfp, "%% Ellipse\n");
	    fprintf(tfp, "n %d %d %d %d 0 360 DrawEllipse ",
		  e->center.x, e->center.y, e->radiuses.x, e->radiuses.y);
	}
	else
	{
	    fprintf(tfp, "%% Rotated Ellipse\n");
	    fprintf(tfp, "gs\n");
	    fprintf(tfp, "%d %d tr\n",e->center.x, e->center.y);
	    fprintf(tfp, "%6.3f rot\n",-e->angle*180.0/M_PI);
	    fprintf(tfp, "n 0 0 %d %d 0 360 DrawEllipse ",
		 e->radiuses.x, e->radiuses.y);
	    /* rotate back so any fill pattern will come out correct */
	    fprintf(tfp, "%6.3f rot\n",e->angle*180.0/M_PI);
	}
	if (e->fill_style != UNFILLED)
	    fill_area(e->fill_style, e->pen_color, e->fill_color,
			e->center.x - e->radiuses.x, e->center.y - e->radiuses.y);
	if (e->thickness > 0)
	    fprintf(tfp, "gs col%d s gr\n", e->pen_color);
	if (e->angle != 0)
	    fprintf(tfp, "gr\n");
	reset_style(e->style, e->style_val);
	if (multi_page)
	   fprintf(tfp, "} bind def\n");
	}


#define	TEXT_PS		"\
/%s%s ff %.2f scf sf\n\
"
void
genps_text(t)
F_text	*t;
{
	unsigned char		*cp;
#ifdef I18N
#define LINE_LENGTH_LIMIT 200
	Boolean composite = False;
	Boolean state_gr = False;
	int chars = 0;
        int gr_chars = 0;
	unsigned char ch;
#endif /* I18N */

	/* ignore hidden text (new for xfig3.2.3/fig2dev3.2.2) */
	if (hidden_text(t))
	    return;

	if (multi_page)
	   fprintf(tfp, "/o%d {", no_obj++);

	/* print any comments prefixed with "%" */
	print_comments("% ",t->comments, "");

#ifdef I18N
	if (enable_composite_font
	    && ((t->flags & PSFONT_TEXT) ? (t->font <= 0 || t->font == 2)
		 : (t->font <= 2))) {
	  composite = True;
	  if (t->font <= 0)
	    fprintf(tfp, TEXT_PS, "CompositeRoman", "", PSFONTMAG(t));
	  else
	    fprintf(tfp, TEXT_PS, "CompositeBold", "", PSFONTMAG(t));
	} else
#endif /* I18N */
	if (PSisomap[t->font+1] == True)
	   fprintf(tfp, TEXT_PS, PSFONT(t), "-iso", PSFONTMAG(t));
	else
	   fprintf(tfp, TEXT_PS, PSFONT(t), "", PSFONTMAG(t));

	fprintf(tfp, "%d %d m\ngs ", t->base_x,  t->base_y);
	if (coord_system == 2)
		fprintf(tfp, "1 -1 sc ");

	if (t->angle != 0)
	   fprintf(tfp, " %.1f rot ", t->angle*180.0/M_PI);
	/* this loop escapes characters '(', ')', and '\' */
	fputc('(', tfp);
#ifdef I18N
	for(cp = (unsigned char *)t->cstring; *cp; cp++) {
	    if (LINE_LENGTH_LIMIT < chars) {
	      fputs("\\\n", tfp);
	      chars = 0;
	    }
	    ch = *cp;
	    if (enable_composite_font && composite) {
	      if (ch & 0x80) {  /* GR */
		if (!state_gr) {
		  fprintf(tfp, "\\377\\001");
		  chars += 8;
		  state_gr = True;
		  gr_chars = 0;
		}
		gr_chars++;
	      } else {  /* GL */
		if (state_gr) {
		  if (gr_chars % 2) {
		    fprintf(stderr, "warning: incomplete multi-byte text: %s\n",
			    t->cstring);
		    fputc('?', tfp);
		  }
		  fprintf(tfp, "\\377\\000");
		  chars += 8;
		  state_gr = False;
		}
	      }
            }
	    if (strchr("()\\", ch)) {
	      fputc('\\', tfp);
	      chars += 1;
	    }
	    if (ch>=0x80) {
	      fprintf(tfp,"\\%o", ch);
	      chars += 4;
	    } else {
	      fputc(ch, tfp);
	      chars += 1;
	    }
	  }
	  if (enable_composite_font && composite && state_gr) {
	    if (gr_chars % 2) {
	      fprintf(stderr, "warning: incomplete multi-byte text: %s\n",
		      t->cstring);
	      fputc('?', tfp);
	    }
	  }
#else
	for(cp = (unsigned char *)t->cstring; *cp; cp++) {
	    if (strchr("()\\", *cp))
		fputc('\\', tfp);
	    if (*cp>=0x80)
		fprintf(tfp,"\\%o", *cp);
	    else
		fputc(*cp, tfp);
		}
#endif /* I18N */
	fputc(')', tfp);

	if ((t->type == T_CENTER_JUSTIFIED) || (t->type == T_RIGHT_JUSTIFIED)){

	  	fprintf(tfp, " dup sw pop ");
		if (t->type == T_CENTER_JUSTIFIED) fprintf(tfp, "2 div ");
		fprintf(tfp, "neg 0 rm ");
		}

	else if ((t->type != T_LEFT_JUSTIFIED) && (t->type != DEFAULT))
		fprintf(stderr, "Text incorrectly positioned\n");

	fprintf(tfp, " col%d sh gr\n", t->color);

	if (multi_page)
	   fprintf(tfp, "} bind def\n");
	}

/* draw arrow from the points array */

static
draw_arrow(obj, arrow, points, npoints, col)
F_line	*obj;
F_arrow	*arrow;
Point	*points;
int	npoints;
int	col;
{
	int i, type;

	fprintf(tfp,"%% arrowhead\n");
	set_linecap(0);			/* butt line cap for arrowheads */
	set_linejoin(0);		/* miter join for sharp points */
	set_linewidth(arrow->thickness);
	fprintf(tfp, "n ");
	for (i=0; i<npoints; i++) {
	    fprintf(tfp, "%d %d ",points[i].x,points[i].y);
	    if (i==0)
		fprintf(tfp, "m ");
	    else
		fprintf(tfp, "l ");
	    if ((i+1)%5 == 0)
		fprintf(tfp,"\n");
	}

	type = arrow->type;
	if (type != 0 && type != 6 && type < 9)  /* old heads, close the path */
	    fprintf(tfp, " cp ");
	if (type != 0) {
	    if (arrow->style == 0)		/* hollow, fill with white */
		fill_area(NUMSHADES-1, WHITE_COLOR, WHITE_COLOR, 0, 0);
	    else if (type < 9)			/* solid, fill with color  */
		fill_area(NUMSHADES-1, col, col, 0, 0);
	}
	fprintf(tfp, " col%d s\n",col);
}

/****************************************************************

 clip_arrows - calculate a clipping region which is the current 
	clipping area minus the polygons at the arrowheads.

 This will prevent the object (line, spline etc.) from protruding
 on either side of the arrowhead Also calculate the arrowheads
 themselves and put the polygons in fpoints[nfpoints] for forward
 arrow and bpoints[nbpoints] for backward arrow.
 The calling routine should first do a "gs" (graphics state save)
 so that it can restore the original clip area later.

****************************************************************/

static
clip_arrows(obj, objtype)
    F_line	   *obj;
    int		    objtype;
{
    int		    fcx1, fcy1, fcx2, fcy2;
    int		    bcx1, bcy1, bcx2, bcy2;
    int		    i,nbndpts;

    /* get current clip area */
    fprintf(tfp," clippath\n");
    /* get points for any forward arrowhead */
    if (obj->for_arrow) {
	if (objtype == O_ARC) {
	    F_arc  *a = (F_arc *) obj;
	    /* last point */
	    lpntx1 = a->point[2].x;
	    lpnty1 = a->point[2].y;
	    compute_arcarrow_angle(a->center.x, a->center.y, 
	    			(double) lpntx1, (double) lpnty1,
				a->direction, a->for_arrow, &lpntx2, &lpnty2);
	}
	calc_arrow(lpntx2, lpnty2, lpntx1, lpnty1, &fcx1, &fcy1, &fcx2, &fcy2,
		   obj->for_arrow, fpoints, &nfpoints, &nbndpts);
	/* set clipping to the *outside* of the first nbndpts points of the 
	   arrowhead and the box surrounding it */
	/* draw the box clockwise */
	fprintf(tfp, "%d %d m %d %d l ",fcx1, fcy1, fcx2, fcy2);
	for (i=nbndpts-1; i>=0; i--) {
	    fprintf(tfp,"%d %d l ",fpoints[i].x,fpoints[i].y);
	}
	fprintf(tfp, "cp\n");
    }
	
    /* get points for any backward arrowhead */
    if (obj->back_arrow) {
	if (objtype == O_ARC) {
	    F_arc  *a = (F_arc *) obj;
	    /* first point */
	    fpntx1 = a->point[0].x;
	    fpnty1 = a->point[0].y;
	    compute_arcarrow_angle(a->center.x, a->center.y,
				(double) fpntx1, (double) fpnty1,
				a->direction ^ 1, a->back_arrow, &fpntx2, &fpnty2);
	}
	calc_arrow(fpntx2, fpnty2, fpntx1, fpnty1, &bcx1, &bcy1, &bcx2, &bcy2,
		    obj->back_arrow, bpoints, &nbpoints, &nbndpts);
	/* set clipping to the *outside* of the first three points of the 
	   arrowhead and the box surrounding it */
	/* draw the box clockwise */
	fprintf(tfp, "%d %d m %d %d l ",bcx1, bcy1, bcx2, bcy2);
	for (i=nbndpts-1; i>=0; i--) {
	    fprintf(tfp,"%d %d l ",bpoints[i].x,bpoints[i].y);
	}
	fprintf(tfp, "cp\n");
    }
    /* intersect the arrowhead clip path(s) with current clip path */
    /* use eoclip so that the intersection with the current path
       guarantees the correct clip path */
    fprintf(tfp, "eoclip\n");
}

/****************************************************************

 calc_arrow - calculate points heading from (x1, y1) to (x2, y2)

 Must pass POINTER to npoints for return value and for c1x, c1y,
 c2x, c2y, which are two points at the end of the arrowhead so:

		|\     + (c1x,c1y)
		|  \
		|    \
 ---------------|      \
		|      /
		|    /
		|  /
		|/     + (c2x,c2y)


 Fills points array with npoints arrowhead coordinates

****************************************************************/

static
calc_arrow(x1, y1, x2, y2, c1x, c1y, c2x, c2y, arrow, points, npoints, nboundpts)
    int		    x1, y1, x2, y2;
    int		   *c1x, *c1y, *c2x, *c2y;
    F_arrow	   *arrow;
    Point	    points[];
    int		   *npoints, *nboundpts;
{
    double	    x, y, xb, yb, dx, dy, l, sina, cosa;
    double	    mx, my;
    double	    ddx, ddy, lpt, tipmv;
    double	    alpha;
    double	    miny, maxy;
    double	    thick;
    int		    xa, ya, xs, ys;
    double	    xt, yt;
    float	    wd = arrow->wid;
    float	    ht = arrow->ht;
    int		    type, style, indx;
    int		    i, np;

    /* types = 0...10 */
    type = arrow->type;
    /* style = 0 (unfilled) or 1 (filled) */
    style = arrow->style;
    /* index into shape array */
    indx = 2*type + style;

    *npoints = 0;
    dx = x2 - x1;
    dy = y1 - y2;
    if (dx==0 && dy==0)
	return;

    /* lpt is the amount the arrowhead extends beyond the end of the
       line because of the sharp point (miter join) */

    tipmv = arrow_shapes[indx].tipmv;
    lpt = 0.0;
    /* lines are made a little thinner in set_linewidth */
    thick = (arrow->thickness <= THICK_SCALE) ? 	
		0.5* arrow->thickness :
		arrow->thickness - THICK_SCALE;
    if (tipmv > 0.0)
        lpt = thick / (2.0 * sin(atan(wd / (tipmv * ht))));
    else if (tipmv == 0.0)
	lpt = thick / 3.0;	 /* types which have blunt end */
    /* (Don't adjust those with tipmv < 0) */

    /* alpha is the angle the line is relative to horizontal */
    alpha = atan2(dy,-dx);

    /* ddx, ddy is amount to move end of line back so that arrowhead point
       ends where line used to */
    ddx = lpt * cos(alpha);
    ddy = lpt * sin(alpha);

    /* move endpoint of line back */
    mx = x2 + ddx;
    my = y2 + ddy;

    l = sqrt(dx * dx + dy * dy);
    sina = dy / l;
    cosa = dx / l;
    xb = mx * cosa - my * sina;
    yb = mx * sina + my * cosa;

    /* (xa,ya) is the rotated endpoint */
    xa =  xb * cosa + yb * sina + 0.5;
    ya = -xb * sina + yb * cosa + 0.5;

    /*
     * We approximate circles with an octagon since, at small sizes,
     * this is sufficient.  I haven't bothered to alter the bounding
     * box calculations.
     */
    miny =  100000.0;
    maxy = -100000.0;
    if (type == 5 || type == 6) {	/* also include half circle */
	double rmag;
	double angle, init_angle, rads;
	double fix_x, fix_y;

	/* get angle of line */
	init_angle = compute_angle(dx,dy);

	/* (xs,ys) is a point the length (height) of the arrowhead BACK from 
	   the end of the shaft */
	/* for the half circle, use 0.0 */
	xs =  (xb-(type==5? ht: 0.0)) * cosa + yb * sina + 0.5;
	ys = -(xb-(type==5? ht: 0.0)) * sina + yb * cosa + 0.5;

	/* calc new (dx, dy) from moved endpoint to (xs, ys) */
	dx = mx - xs;
	dy = my - ys;
	/* radius */
	rmag = ht/2.0;
	fix_x = xs + (dx / (double) 2.0);
	fix_y = ys + (dy / (double) 2.0);
	/* choose number of points for circle - 20+mag/4 points */
	np = round(mag/4.0) + 20;
	/* full or half circle? */
	rads = (type==5? M_2PI: M_PI);

	if (type == 5) {
	    init_angle = 5.0*M_PI_2 - init_angle;
	    /* np/2 points in the forward part of the circle for the line clip area */
	    *nboundpts = np/2;
	    /* full circle */
	    rads = M_2PI;
	} else {
	    init_angle = 3.0*M_PI_2 - init_angle;
	    /* no points in the line clip area */
	    *nboundpts = 0;
	    /* half circle */
	    rads = M_PI;
	}
	/* draw the half or full circle */
	for (i = 0; i < np; i++) {
	    if (type == 5)
		angle = init_angle - (rads * (double) i / (double) np);
	    else
		angle = init_angle - (rads * (double) i / (double) (np-1));
	    x = fix_x + round(rmag * cos(angle));
	    points[*npoints].x = x;
	    y = fix_y + round(rmag * sin(angle));
	    points[*npoints].y = y;
	    miny = min2(y, miny);
	    maxy = max2(y, maxy);
	    (*npoints)++;
	}
	x = 2.0*THICK_SCALE;
	y = rmag;
	xt =  x*cosa + y*sina + x2;
	yt = -x*sina + y*cosa + y2;
	*c1x = xt;
	*c1y = yt;
	y = -rmag;
	xt =  x*cosa + y*sina + x2;
	yt = -x*sina + y*cosa + y2;
	*c2x = xt;
	*c2y = yt;
    } else {
	/* 3 points in the arrowhead that define the line clip part */
	*nboundpts = 3;
	np = arrow_shapes[indx].numpts;
	for (i=0; i<np; i++) {
	    x = arrow_shapes[indx].points[i].x * ht;
	    y = arrow_shapes[indx].points[i].y * wd;
	    miny = min2(y, miny);
	    maxy = max2(y, maxy);
	    xt =  x*cosa + y*sina + xa;
	    yt = -x*sina + y*cosa + ya;
	    points[*npoints].x = xt;
	    points[*npoints].y = yt;
	    (*npoints)++;
	}
	x = arrow_shapes[indx].points[arrow_shapes[indx].tipno].x * ht + THICK_SCALE;
	y = maxy;
	xt =  x*cosa + y*sina + x2;
	yt = -x*sina + y*cosa + y2;
	*c1x = xt;
	*c1y = yt;
	y = miny;
	xt =  x*cosa + y*sina + x2;
	yt = -x*sina + y*cosa + y2;
	*c2x = xt;
	*c2y = yt;
    }
}

/********************* COMPUTE ANGLE ************************

Input arguments :
	(dx,dy) : the vector (0,0)(dx,dy)
Output arguments : none
Return value : the angle of the vector in the range [0, 2PI)

*************************************************************/

double
compute_angle(dx, dy)		/* compute the angle between 0 to 2PI  */
    double	    dx, dy;
{
    double	    alpha;

    if (dx == 0) {
	if (dy > 0)
	    alpha = M_PI_2;
	else
	    alpha = 3 * M_PI_2;
    } else if (dy == 0) {
	if (dx > 0)
	    alpha = 0;
	else
	    alpha = M_PI;
    } else {
	alpha = atan(dy / dx);	/* range = -PI/2 to PI/2 */
	if (dx < 0)
	    alpha += M_PI;
	else if (dy < 0)
	    alpha += M_2PI;
    }
    return (alpha);
}

static
arc_tangent(x1, y1, x2, y2, direction, x, y)
double	x1, y1, x2, y2;
int	*x, *y;
int	direction;
{
	if (direction==0) { /* counter clockwise  */
	    *x = round(x2 + (y2 - y1));
	    *y = round(y2 - (x2 - x1));
	    }
	else {
	    *x = round(x2 - (y2 - y1));
	    *y = round(y2 + (x2 - x1));
	    }
	}

/* Computes a point on a line which is a chord to the arc specified by */
/* center (x1,y1) and endpoint (x2,y2), where the chord intersects the */
/* arc arrow->ht from the endpoint.                                    */
/* May give strange values if the arrow.ht is larger than about 1/4 of */
/* the circumference of a circle on which the arc lies.                */

compute_arcarrow_angle(x1, y1, x2, y2, direction, arrow, x, y)
    double	 x1, y1;
    double	 x2, y2;
    int		*x, *y;
    int		 direction;
    F_arrow	*arrow;
{
    double	 r, alpha, beta, dy, dx;
    double	 lpt,h;
    double	 thick;

    dy=y2-y1;
    dx=x2-x1;
    r=sqrt(dx*dx+dy*dy);
    h = (double) arrow->ht;
    thick <= THICK_SCALE ? 	/* lines are made a little thinner in set_linewidth */
		0.5* arrow->thickness :
		arrow->thickness - THICK_SCALE;
    /* lpt is the amount the arrowhead extends beyond the end of the line */
    lpt = thick/2.0/(arrow->wid/h/2.0);
    /* add this to the length */
    h += lpt;

    /* radius too small for this method, use normal method */
    if (h > 2.0*r) {
	arc_tangent(x1,y1,x2,y2,direction,x,y);
	return;
    }

    beta=atan2(dy,dx);
    if (direction) {
	alpha = 2*asin(h/2.0/r);
    } else {
	alpha = -2*asin(h/2.0/r);
    }

    *x=round(x1+r*cos(beta+alpha));
    *y=round(y1+r*sin(beta+alpha));
}

/* uses eofill (even/odd rule fill) */
/* ulx and uly define the upper-left corner of the object for pattern alignment */

static
fill_area(fill, pen_color, fill_color, ulx, uly)
int fill, pen_color, fill_color, ulx, uly;
{
   float pen_r, pen_g, pen_b, fill_r, fill_g, fill_b;

   /* get the rgb values for the fill pattern (if necessary) */
   if (fill_color < NUM_STD_COLS) {
	fill_r=rgbcols[fill_color>0? fill_color: 0].r;
	fill_g=rgbcols[fill_color>0? fill_color: 0].g;
	fill_b=rgbcols[fill_color>0? fill_color: 0].b;
   } else {
	fill_r=user_colors[fill_color-NUM_STD_COLS].r/255.0;
	fill_g=user_colors[fill_color-NUM_STD_COLS].g/255.0;
	fill_b=user_colors[fill_color-NUM_STD_COLS].b/255.0;
   }
   if (pen_color < NUM_STD_COLS) {
	pen_r=rgbcols[pen_color>0? pen_color: 0].r;
	pen_g=rgbcols[pen_color>0? pen_color: 0].g;
	pen_b=rgbcols[pen_color>0? pen_color: 0].b;
   } else {
	pen_r=user_colors[pen_color-NUM_STD_COLS].r/255.0;
	pen_g=user_colors[pen_color-NUM_STD_COLS].g/255.0;
	pen_b=user_colors[pen_color-NUM_STD_COLS].b/255.0;
   }

   if (fill_color <= 0) {   /* use gray levels for default and black */
	if (fill < NUMSHADES+NUMTINTS)
	    fprintf(tfp, "gs %.2f setgray ef gr ", 1.0 - SHADEVAL(fill));
	/* one of the patterns */
	else {
	    int patnum = fill-NUMSHADES-NUMTINTS+1;
	    fprintf(tfp, "gs /PC [[%.2f %.2f %.2f] [%.2f %.2f %.2f]] def\n",
			fill_r, fill_g, fill_b, pen_r, pen_g, pen_b);
	    fprintf(tfp, "%.2f %.2f sc P%d [%d 0 0 %d %.2f %.2f]  PATmp PATsp ef gr PATusp ",
			THICK_SCALE, THICK_SCALE, patnum,
			patmat[patnum-1][0],patmat[patnum-1][1],
		        (float)ulx/THICK_SCALE, (float)uly/THICK_SCALE);
	}
   /* color other than default(black) */
   } else {
	/* a shade */
	if (fill < NUMSHADES)
	    fprintf(tfp, "gs col%d %.2f shd ef gr ", fill_color, SHADEVAL(fill));
	/* a tint */
	else if (fill < NUMSHADES+NUMTINTS)
	    fprintf(tfp, "gs col%d %.2f tnt ef gr ", fill_color, TINTVAL(fill));
	/* one of the patterns */
	else {
	    int patnum = fill-NUMSHADES-NUMTINTS+1;
	    fprintf(tfp, "gs /PC [[%.2f %.2f %.2f] [%.2f %.2f %.2f]] def\n",
			fill_r, fill_g, fill_b, pen_r, pen_g, pen_b);
	    fprintf(tfp, "%.2f %.2f sc P%d [%d 0 0 %d %.2f %.2f] PATmp PATsp ef gr PATusp ",
			THICK_SCALE, THICK_SCALE, patnum,
			patmat[patnum-1][0],patmat[patnum-1][1],
		        (float)ulx/THICK_SCALE, (float)uly/THICK_SCALE);
	}
   }
}

/* define standard colors as "col##" where ## is the number */
genps_std_colors()
{
    int i;
    for (i=0; i<NUM_STD_COLS; i++) {
	fprintf(tfp, "/col%d {%.3f %.3f %.3f srgb} bind def\n", i,
		rgbcols[i].r, rgbcols[i].g, rgbcols[i].b);
    }
}
	
/* define user colors as "col##" where ## is the number */
genps_usr_colors()
{
    int i;
    for (i=0; i<num_usr_cols; i++) {
	fprintf(tfp, "/col%d {%.3f %.3f %.3f srgb} bind def\n", i+NUM_STD_COLS,
		user_colors[i].r/255.0, user_colors[i].g/255.0, user_colors[i].b/255.0);
    }
}
	
static
iso_text_exist(ob)
F_compound      *ob;
{
   F_compound	*c;
   F_text          *t;
   unsigned char   *s;

   if (ob->texts != NULL)
   {
      for (t = ob->texts; t != NULL; t = t->next)
      {
	 for (s = (unsigned char*)t->cstring; *s != '\0'; s++)
	 {
	    /* look for characters >= 128 or ASCII '-' */
	    if ((*s>127) || (*s==45))
		return 1;
	 }
      }
   }

   for (c = ob->compounds; c != NULL; c = c->next) {
	if (iso_text_exist(c))
	    return 1;
   }
   return 0;
}

static
encode_all_fonts(ob)
F_compound	*ob;
{
   F_compound *c;
   F_text     *t;

   if (ob->texts != NULL) {
	for (t = ob->texts; t != NULL; t = t->next)
	    if (PSisomap[t->font+1] == False) {
		fprintf(tfp, "/%s /%s-iso isovec ReEncode\n", PSFONT(t), PSFONT(t));
		PSisomap[t->font+1] = True;
	}
   }

   for (c = ob->compounds; c != NULL; c = c->next) {
	encode_all_fonts(c);
   }
}

static
ellipse_exist(ob)
F_compound	*ob;
{
	F_compound	*c;

	if (NULL != ob->ellipses) 
		return 1;

	for (c = ob->compounds; c != NULL; c = c->next) {
	    if (ellipse_exist(c))
		return 1;
	}

	return 0;
	}

static
approx_spline_exist(ob)
F_compound	*ob;
{
	F_spline	*s;
	F_compound	*c;

	for (s = ob->splines; s != NULL; s = s->next) {
	    if (approx_spline(s))
		return 1;
	}

	for (c = ob->compounds; c != NULL; c = c->next) {
	    if (approx_spline_exist(c))
		return 1;
	}

	return 0;
	}

#ifdef USE_XPM

/* lookup the named colors referenced in the colortable passed */
/* total colors in the table are "ncols" */
/* This is called from the XPM image import section above */

/* lookup color names and return rgb values from X11
   RGB database file (e.g. /usr/lib/X11/rgb.XXX) */

convert_xpm_colors(cmap, coltabl, ncols)
	unsigned char cmap[3][MAXCOLORMAPSIZE];
	XpmColor *coltabl;
	int	  ncols;
{
	int	i,j;
	char	*name;
	RGB	rgb;

	/* look through each entry in the colortable for the named colors */
	for (i=0; i<ncols; i++) {
	    name = (coltabl+i)->c_color;
	    /* get the rgb values from the name */
	    if (lookup_db_color(name, &rgb) < 0)
		fprintf(stderr,"Can't parse color '%s', using black.\n",name);
	    cmap[0][i] = (unsigned char) rgb.red;
	    cmap[1][i] = (unsigned char) rgb.green;
	    cmap[2][i] = (unsigned char) rgb.blue;
	}
}

#endif /* USE_XPM */

struct
driver dev_ps = {
     	genps_option,
	genps_start,
	genps_arc,
	genps_ellipse,
	genps_line,
	genps_spline,
	genps_text,
	genps_end,
	INCLUDE_TEXT
};

/* eps is just like ps except with no: pages, pagesize, orientation, offset */

struct
driver dev_eps = {
     	geneps_option,
	genps_start,
	genps_arc,
	genps_ellipse,
	genps_line,
	genps_spline,
	genps_text,
	genps_end,
	INCLUDE_TEXT
};
