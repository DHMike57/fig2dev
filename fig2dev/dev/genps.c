/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1985 Supoj Sutantavibul
 * Copyright (c) 1991 Micah Beck
 * Parts Copyright (c) 1994 Brian V. Smith
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
#include <stdio.h>
#include <math.h>
#include <pwd.h>
#include <errno.h>
#ifndef __NetBSD__
#if (! (defined(BSD) && (BSD >= 199306)))
extern char *sys_errlist[];
#endif
#endif
#include "pi.h"
#include "fig2dev.h"
#include "object.h"
#include "psfonts.h"
#include <string.h>
#include <time.h>

/* for the xpm package */
#ifdef USE_XPM
#include <xpm.h>
int	XpmReadFileToXpmImage();
#endif

#ifdef I18N
#include <stdlib.h>
extern Boolean support_i18n;  /* enable i18n support? */
static Boolean enable_composite_font = FALSE;
#endif /* I18N */

/* for the version nubmer */
#include "../../patchlevel.h"

/* include the PostScript preamble, patterns etc */
#include "genps.h"

int	ReadFromBitmapFile();

typedef struct _point
{
    int x,y;
} Point;

#define		POINT_PER_INCH		72
#define		ULIMIT_FONT_SIZE	300

#define		min(a, b)		(((a) < (b)) ? (a) : (b))

int		pagewidth = -1;
int		pageheight = -1;
int		xoff=0;
int		yoff=0;
static int	coord_system;
static int	resolution;
Boolean		show_page = FALSE;
static double	cur_thickness = 0.0;
static int	cur_joinstyle = 0;
static int	cur_capstyle = 0;
int		pages;
int		no_obj = 0;

/* arrowhead arrays */
Point		bpoints[5], fpoints[5];
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

#define SHADEVAL(F)	1.0*(F)/(NUMSHADES-1)
#define TINTVAL(F)	1.0*(F-NUMSHADES+1)/NUMTINTS

struct	_rgb {
	float r, g, b;
	}
    rgbcols[32] = {
	{0, 0, 0},
	{0, 0, 1},
	{0, 1, 0},
	{0, 1, 1},
	{1, 0, 0},
	{1, 0, 1},
	{1, 1, 0},
	{1, 1, 1},
	{0, 0, .56},
	{0, 0, .69},
	{0, 0, .82},
	{.53, .81, 1},
	{0, .56, 0},
	{0, .69, 0},
	{0, .82, 0},
	{0, .56, .56},
	{0, .69, .69},
	{0, .82, .82},
	{.56, 0, 0},
	{.69, 0, 0},
	{.82, 0, 0},
	{.56, 0, .56},
	{.69, 0, .69},
	{.82, 0, .82},
	{.5, .19, 0},
	{.63, .25, 0},
	{.75, .38, 0},
	{1, .5, .5},
	{1, .63, .63},
	{1, .75, .75},
	{1, .88, .88},
	{1, .84, 0}
    };

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

static double		scalex, scaley;
static double		origx, origy;

void
genps_option(opt, optarg)
char opt;
char *optarg;
{
	int i;

	switch (opt) {

	case 'f':			/* default font name */
		for ( i = 1; i <= MAX_PSFONT; i++ )
			if ( !strcmp(optarg, PSfontnames[i]) ) break;

		if ( i > MAX_PSFONT )
			fprintf(stderr,
			    "warning: non-standard font name %s\n", optarg);

	    	psfontnames[0] = psfontnames[1] = optarg;
	    	PSfontnames[0] = PSfontnames[1] = optarg;
	    	break;

	case 'c':			/* center figure */
	    	center = TRUE;
		centerspec = TRUE;	/* user-specified */
		break;

	case 'e':			/* don't center ('e' means edge) figure */
	    	center = FALSE;
		centerspec = TRUE;	/* user-specified */
		break;

	case 'M':			/* multi-page option */
		multi_page = TRUE;
		multispec = TRUE;	/* user has overridden anything in file */
		break;

	case 'S':			/* turn off multi-page option */
		multi_page = FALSE;
		multispec = TRUE;	/* user has overridden anything in file */
		break;

	case 'P':			/* add showpage */
		show_page = TRUE;
		break;

      	case 'm':			/* magnification (already parsed in main) */
		magspec = TRUE;		/* user-specified */
		break;

	/* don't do anything for language and font size (already parsed in main) */
      	case 'L':			/* language */
	case 's':			/* default font size */
		break;

	case 'n':			/* name to put in the "Title:" spec */
		name = optarg;
		break;

      	case 'l':			/* landscape mode */
		landscape = TRUE;	/* override the figure file setting */
		orientspec = TRUE;	/* user-specified */
		break;

      	case 'p':			/* portrait mode */
		landscape = FALSE;	/* override the figure file setting */
		orientspec = TRUE;	/* user-specified */
		break;

	case 'x':			/* x offset on page */
		xoff = atoi(optarg);
		break;

	case 'y':			/* y offset on page */
		yoff = atoi(optarg);
		break;

	case 'z':			/* papersize */
		(void) strcpy (papersize, optarg);
		paperspec = TRUE;	/* user-specified */
		break;

	default:
		put_msg(Err_badarg, opt, "ps");
		exit(1);
		break;
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
	int		clipx, clipy;
	struct paperdef	*pd;

	resolution = objects->nwcorner.x;
	coord_system = objects->nwcorner.y;
	scalex = scaley = mag * POINT_PER_INCH / (double)resolution;
	/* if metric, adjust scale for difference between 
	   FIG PIX/CM (450) and actual (472.44) */
	if (metric) {
		scalex *= 80.0/76.2;
		scaley *= 80.0/76.2;
	}

	/* convert to point unit */
	llx = (int)floor(llx * scalex); lly = (int)floor(lly * scaley);
	/* save upper bounds before scaling for clipping later */
	clipx = urx;
	clipy = ury;
	urx = (int)ceil(urx * scalex); ury = (int)ceil(ury * scaley);

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

	if (landscape) {
	    if (show_page) {
		itmp = pageheight; pageheight = pagewidth; pagewidth = itmp;
		itmp = llx; llx = lly; lly = itmp;
		itmp = urx; urx = ury; ury = itmp;
	    }
	}
	if (show_page) {
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
	} else {
	    origx = -llx;
	    origy = (landscape && show_page) ? -lly : ury;
	}

	/* finally, adjust by any offset the user wants */
	if (landscape && show_page) {
	    origx += yoff;
	    origy += xoff;
	} else {
	    origx += xoff;
	    origy += yoff;
	}

	if (show_page)
	    fprintf(tfp, "%%!PS-Adobe-2.0\n");		/* PostScript magic strings */
	else
	    fprintf(tfp, "%%!PS-Adobe-2.0 EPSF-2.0\n");	/* Encapsulated PostScript */
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

	if (!center) {
	   if (landscape && show_page)
		pages = (urx/pageheight+1)*(ury/pagewidth+1);
	   else
		pages = (urx/pagewidth+1)*(ury/pageheight+1);
	} else {
	   pages = 1;
	}
	if (landscape && show_page) {
	   fprintf(tfp, "%%%%Orientation: Landscape\n");
	   fprintf(tfp, "%%%%BoundingBox: %d %d %d %d\n",
	      (int)origx+llx, (int)origy+lly, (int)origx+urx, (int)origy+ury);
	} else {
	   fprintf(tfp, "%%%%Orientation: Portrait\n");
	   fprintf(tfp, "%%%%BoundingBox: %d %d %d %d\n",
	      (int)origx+llx, (int)origy-ury, (int)origx+urx, (int)origy-lly);
	}
	fprintf(tfp, "%%%%Pages: %d\n", show_page ? pages : 0 );
	fprintf(tfp, "%%%%BeginSetup\n");
	/* only include a pagesize command if not EPS */
	if (show_page) {
	    fprintf(tfp, "%%%%IncludeFeature: *PageSize %s\n", papersize);
	}
	fprintf(tfp, "%%%%EndSetup\n");

	/* put in the magnification for information purposes */
	fprintf(tfp, "%%%%Magnification: %.4f\n",mag);

	fprintf(tfp, "%%%%EndComments\n");

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
	fprintf(tfp, "%.1f %.1f translate\n", origx, origy);
	/* also flip y if necessary, but *only* if to PostScript and not EPS */
	if (landscape && show_page) {
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
	  if (libdir == NULL) libdir = FIG2DEV_LIBDIR;
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
	      if (strstr(str, "CompositeRoman")) enable_composite_font = TRUE;
	      fputs(str, tfp);
	    }
	    fclose(fp);
	  }
	}
#endif /* I18N */
	
	fprintf(tfp, "%s\n", END_PROLOG);
	fprintf(tfp, "$F2psBegin\n");
	fprintf(tfp, "10 setmiterlimit\n");	/* make like X server (11 degrees) */
	/* set initial clipping area to size of the bounding box plus a margin
	   (this is needed for later clipping by arrowheads */
	fprintf(tfp, "n %d %d m %d %d l %d %d l %d %d l cp clip\n",
			-1000,clipy+1000,-1000,-1000,clipx+1000,-1000,clipx+1000,clipy+1000);

 	if ( multi_page ) {
	    fprintf(tfp, "initmatrix\n");
	} else {
	    fprintf(tfp, " %.5f %.5f sc\n", scalex, scaley );
	    if (show_page)
		fprintf(tfp,"%%%%Page: 1 1\n");
	}
}

void
genps_end()
{
    double dx,dy;
    int i, page;
    int h,w;

    if (multi_page) {
       page = 1;
       h = ((landscape && show_page)? pagewidth: pageheight);
       w = ((landscape && show_page)? pageheight: pagewidth);
       for (dy=0; dy < (ury-h*0.1); dy += h*0.9) {
	 for (dx=0; dx < (urx-w*0.1); dx += w*0.9) {
	    fprintf(tfp, "%%%%Page: %d %d\n",page,page);
	    fprintf(tfp, "gs\n");
	    fprintf(tfp,"%.1f %.1f tr", 
		-dx, ((landscape && show_page)? -dy:(dy+h*0.9)));
	    if (landscape && show_page) {
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
    if (!multi_page && show_page)
	fprintf(tfp, "showpage\n");
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

void
genps_line(l)
F_line	*l;
{
	F_point		*p, *q;
	/* JNT */
	int		radius;
	int		i;
	FILE		*picf;
	char		buf[512];
	char		*cp;
	int		xmin,xmax,ymin,ymax;
	int		pic_w, pic_h;
	Boolean		namedcol;
	float		hf_wid;
	
	if (multi_page)
	   fprintf(tfp, "/o%d {", no_obj++);
	fprintf(tfp, "%% Polyline\n");
	if (l->type != T_PIC_BOX) {  /* pic object has no line thickness */
		set_linejoin(l->join_style);
		set_linecap(l->cap_style);
		set_linewidth((double)l->thickness);
	}
	p = l->points;
	q = p->next;
	if (q == NULL) { /* A single point line */
	    /*set_linecap(0);	/* make CAP_BUTT */
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
	} else if (l->type == T_PIC_BOX) {  /* postscript (eps), XPM, X bitmap or GIF file */
	  /* PICTURE OBJECT */
		int             dx, dy, rotation;
		int		llx, lly, urx, ury;
		double          fllx, flly, furx, fury;
#ifdef USE_XPM
		XpmImage	xpmimage;
#endif

		dx = l->points->next->next->x - l->points->x;
		dy = l->points->next->next->y - l->points->y;
		rotation = 0;
		if (dx < 0 && dy < 0)
			   rotation = 180.0;
		else if (dx < 0 && dy >= 0)
			   rotation = 90;
		else if (dy < 0 && dx >= 0)
			   rotation = 270;

		fprintf(tfp, "%%\n");

		/* first try for a X Bitmap file format */
		if (ReadFromBitmapFile(l->pic->file, &dx, &dy, &l->pic->bitmap)) {
			fprintf(tfp, "%% Begin Imported X11 Bitmap File: %s\n", l->pic->file);
			fprintf(tfp, "%%\n");
			l->pic->subtype = P_XBM;
			llx = lly = 0;
			urx = dx;		/* size of bitmap from the file */
			ury = dy;

#ifdef USE_XPM
		/* not X11 bitmap, try XPM */
		} else if (XpmReadFileToXpmImage(l->pic->file, &xpmimage, NULL)
					== XpmSuccess) {
			/* yes, say so */
			fprintf(tfp, "%% Begin Imported XPM File: %s\n", l->pic->file);
			/* and set type */
			l->pic->subtype = P_XPM;

			llx = lly = 0;
			urx = xpmimage.width;	/* size of image from the file */
			ury = xpmimage.height;

#endif /* USE_XPM */
		/* not XPM, try GIF */
		} else if (read_gif(l->pic) > 0) {
			/* yes, say so */
			fprintf(tfp, "%% Begin Imported GIF File: %s\n", l->pic->file);

			llx = lly = 0;
			urx = l->pic->bit_size.x;	/* size of image from the file */
			ury = l->pic->bit_size.y;

		/* not GIF, try JPEG (JFIF) */
#ifdef USE_JPEG
		} else if (read_jpg(l->pic) > 0) {
			/* yes, say so */
			fprintf(tfp, "%% Begin Imported JPEG File: %s\n", l->pic->file);

			llx = lly = 0;
			urx = l->pic->bit_size.x;	/* size of image from the file */
			ury = l->pic->bit_size.y;

#endif /* USE_JPEG */
		/* none of the above, try EPS */
		} else {
			fprintf(tfp, "%% Begin Imported EPS File: %s\n", l->pic->file);
			fprintf(tfp, "%%%%BeginDocument: %s\n", l->pic->file);
			fprintf(tfp, "%%\n");

			if ((picf=open_picfile(l->pic->file, &filtype)) == NULL) {
			    fprintf(stderr, "Unable to open EPS file '%s': error: %s (%d)\n",
					l->pic->file, sys_errlist[errno],errno);
			    return;
			}
			while (fgets(buf, 512, picf) != NULL) {
			    char *c;

			    if (!strncmp(buf, "%%BoundingBox:", 14)) {
				c=buf+14;
				/* skip past white space */
				while (*c == ' ' || *c == '\t') 
				    c++;
				if (strncmp(c,"(atend)",7)) {	/* make sure not an (atend) */
				    if (sscanf(c, "%lf %lf %lf %lf",
						&fllx, &flly, &furx, &fury) < 4) {
					fprintf(stderr,"Bad EPS bitmap file: %s\n", l->pic->file);
					close_picfile(picf,filtype);
					return;
				    }
				    l->pic->subtype = P_EPS;
				    llx= floor(fllx);
				    lly= floor(flly);
				    urx= ceil(furx);
				    ury= ceil(fury);
				    break;
				}
			    }
			}
			close_picfile(picf,filtype);
		}

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
		    fprintf(tfp, "n %d %d m %d %d l %d %d l %d %d l cp clip\n",
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
			for (i=0; i<ury; i++) {			/* for each row */
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
			char	 *c, tmpc[8];
			int	  r,g,b;
			unsigned char *cdata, *cp;
			unsigned int  *dp;

			/* start with width and height */
			wid = xpmimage.width;
			ht = xpmimage.height;
			fprintf(tfp, "%% Pixmap image follows:\n");
			/* scale for size in bits */
			fprintf(tfp, "%d %d sc\n", urx, ury);
			/* modify colortable entries to make consistent */
			coltabl = xpmimage.colorTable;
			namedcol = FALSE;
			/* convert the color defs to a consistent #rrggbb */
			for (i=0; i<xpmimage.ncolors; i++) {
				c = (coltabl + i)->c_color;
				if (c[0] != '#') {		/* named color, set flag */
					namedcol = TRUE;
				/* want to make #RRGGBB from possibly other formats */
				} else if (strlen(c) == 4) {	/* #rgb */
					sprintf(tmpc,"#%.1s%.1s%.1s%.1s%.1s%.1s",
						&c[1],&c[1],&c[2],&c[2],&c[3],&c[3]);
					strcpy((coltabl + i)->c_color,tmpc);
				} else if (strlen(c) == 10) {	/* #rrrgggbbb */
					sprintf(tmpc,"#%.2s%.2s%.2s",&c[1],&c[4],&c[7]);
					strcpy((coltabl + i)->c_color,tmpc);
				} else if (strlen(c) == 13) {	/* #rrrrggggbbbb */
					sprintf(tmpc,"#%.2s%.2s%.2s",&c[1],&c[5],&c[9]);
					strcpy((coltabl + i)->c_color,tmpc);
				}
			}
			/* go lookup the named colors' rgb values */
			if (namedcol)
				convert_names(coltabl,xpmimage.ncolors);
			/* now make separate Red, Green and Blue color arrays */
			for (i=0; i<xpmimage.ncolors; i++) {
			    c = (coltabl + i)->c_color;
			    if (sscanf(c, "#%02x%02x%02x", &r,&g,&b) != 3)
				    fprintf(stderr,"Error parsing color %s\n",c);
			    l->pic->cmap[0][i] = (unsigned char) r;
			    l->pic->cmap[1][i] = (unsigned char) g;
			    l->pic->cmap[2][i] = (unsigned char) b;
			}
			/* and convert the integer data to unsigned char */
			dp = xpmimage.data;
			if ((cdata = (unsigned char *)
			     malloc(wid*ht*sizeof(unsigned char))) == NULL) {
				fprintf(stderr,"can't allocate space for XPM image\n");
				return;
			}
			cp = cdata;
			for (i=0; i<wid*ht; i++)
			    *cp++ = (unsigned char) *dp++;
				
			/* now write out the image data in a compressed form */
			(void) PSencode(tfp, wid, ht, xpmimage.ncolors,
				l->pic->cmap[0], l->pic->cmap[1], l->pic->cmap[2], 
				cdata);
			/* and free up the space */
			free(cdata);
			XpmFreeXpmImage(&xpmimage);
#endif /* USE_XPM */

		/* GIF or JPEG file */
		} else if (l->pic->subtype == P_GIF || l->pic->subtype == P_JPEG) {
			int		 wid, ht;

			/* start with width and height */
			wid = l->pic->bit_size.x;
			ht = l->pic->bit_size.y;
			if (l->pic->subtype == P_GIF)
			    fprintf(tfp, "%% GIF image follows:\n");
			else
			    fprintf(tfp, "%% JPEG image follows:\n");
			/* scale for size in bits */
			fprintf(tfp, "%d %d sc\n", urx, ury);
			/* now write out the image data in a compressed form */
			(void) PSencode(tfp, wid, ht, l->pic->numcols,
				l->pic->cmap[0], l->pic->cmap[1], l->pic->cmap[2], 
				l->pic->bitmap);

		/* EPS file */
		} else {
		    int i;
		    fprintf(tfp, "%% EPS file follows:\n");
		    if ((picf=open_picfile(l->pic->file, &filtype)) == NULL) {
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
		    fprintf(tfp, "cleartomark\n");
		    fprintf(tfp, "countdictstack exch sub { end } repeat\n");
		}

		/* restore vm and gsave */
		fprintf(tfp, "restore grestore\n");
		fprintf(tfp, "%%\n");
		fprintf(tfp, "%% End Imported PIC File: %s\n", l->pic->file);
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
		i=0;
		while (q->next != NULL) {
		    p = q;
		    q = q->next;
		    fprintf(tfp, " %d %d l", p->x, p->y);
 	    	    if (!((++i)%5))
			fprintf(tfp, "\n");
		}
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
	Boolean		first = TRUE;

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

	/* set clipping for any arrowheads */
	fprintf(tfp, "gs ");
	if (a->for_arrow || a->back_arrow)
	    clip_arrows(a, O_ARC);

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

	/* reset clipping */
	fprintf(tfp," gr\n");
	reset_style(a->style, a->style_val);

	/* now draw the arrowheads, if any */
	if (a->type != T_PIE_WEDGE_ARC) {
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
	    fprintf(tfp, "%6.3f rot\n",-e->angle*180/M_PI);
	    fprintf(tfp, "n 0 0 %d %d 0 360 DrawEllipse ",
		 e->radiuses.x, e->radiuses.y);
	    /* rotate back so any fill pattern will come out correct */
	    fprintf(tfp, "%6.3f rot\n",e->angle*180/M_PI);
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
	Boolean composite = FALSE;
	Boolean state_gr = FALSE;
	int chars = 0;
        int gr_chars = 0;
	unsigned char ch;
#endif /* I18N */

	if (multi_page)
	   fprintf(tfp, "/o%d {", no_obj++);
#ifdef I18N
	if (enable_composite_font && (t->font <= 0 || t->font == 2)) {
	  composite = TRUE;
	  if (t->font <= 0)
	    fprintf(tfp, TEXT_PS, "CompositeRoman", "", PSFONTMAG(t));
	  else
	    fprintf(tfp, TEXT_PS, "CompositeBold", "", PSFONTMAG(t));
	} else
#endif /* I18N */
	if (PSisomap[t->font+1] == TRUE)
	   fprintf(tfp, TEXT_PS, PSFONT(t), "-iso", PSFONTMAG(t));
	else
	   fprintf(tfp, TEXT_PS, PSFONT(t), "", PSFONTMAG(t));

	fprintf(tfp, "%d %d m\ngs ", t->base_x,  t->base_y);
	if (coord_system == 2)
		fprintf(tfp, "1 -1 sc ");

	if (t->angle != 0)
	   fprintf(tfp, " %.1f rot ", t->angle*180/M_PI);
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
		  state_gr = TRUE;
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
		  state_gr = FALSE;
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
	int i;

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
	}

	if (arrow->type != 0) {		/* close the path and fill */
	    fprintf(tfp, " cp ");
	    if (arrow->style == 0)		/* hollow, fill with white */
		fill_area(NUMSHADES-1, WHITE_COLOR, WHITE_COLOR, 0, 0);
	    else			/* solid, fill with color  */
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
    int		    i;

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
		   obj->thickness, obj->for_arrow, fpoints, &nfpoints);
	/* set clipping to the *outside* of the first three points of the 
	   arrowhead and the box surrounding it */
	/* draw the box clockwise */
	for (i=0; i<3; i++) {
	    fprintf(tfp,"%d %d %c ",fpoints[i].x,fpoints[i].y,i==0? 'm':'l');
	}
	/* clip path for the forward arrow */
	fprintf(tfp, "%d %d l %d %d l cp\n",fcx2, fcy2, fcx1, fcy1);
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
		    obj->thickness, obj->back_arrow, bpoints, &nbpoints);
	/* set clipping to the *outside* of the first three points of the 
	   arrowhead and the box surrounding it */
	/* draw the box clockwise */
	for (i=0; i<3; i++) {
	    fprintf(tfp,"%d %d %c ",bpoints[i].x,bpoints[i].y,i==0? 'm':'l');
	}
	/* clip path for the backward arrow */
	fprintf(tfp, "%d %d l %d %d l cp\n",bcx2, bcy2, bcx1, bcy1);
    }
    /* intersect the arrowhead clip path(s) with current clip path */
    fprintf(tfp, "clip\n");
}

/****************************************************************

 calc_arrow - calculate points heading from (x1, y1) to (x2, y2)

 Must pass POINTER to npoints for return value and for c1x, c1y,
 c2x, c2y, which are two points at the end of the arrowhead such
 that xc, yc, c1x, c1y, c2x, c2y and xd, yd form the bounding
 rectangle of the arrowhead.

 Fills points array with npoints arrowhead coordinates

****************************************************************/

static
calc_arrow(x1, y1, x2, y2, c1x, c1y, c2x, c2y, objthick, arrow, points, npoints)
    int		    x1, y1, x2, y2;
    int		   *c1x, *c1y, *c2x, *c2y;
    int		    objthick;
    F_arrow	   *arrow;
    Point	    points[];
    int		   *npoints;
{
    double	    x, y, xb, yb, dx, dy, l, sina, cosa;
    double	    xxb;
    double	    mx, my;
    double	    ddx, ddy, lpt;
    double	    alpha;
    int		    xc, yc, xd, yd, xs, ys;
    int		    xg, yg, xh, yh;
    float	    wid = arrow->wid;
    float	    ht = arrow->ht;
    int		    type = arrow->type;
    int		    style = arrow->style;

    *npoints = 0;
    *c1x=x1;
    *c1y=y1;
    *c2x=x2;
    *c2y=y2;
    dx = x2 - x1;
    dy = y1 - y2;
    /* two points are the same, can't determine an arrowhead */
    if (dx==0 && dy==0)
	return;

    /* lpt is the amount the arrowhead extends beyond the end of the
       line because of the sharp point (miter join) */
    lpt = arrow->thickness/2.0/(wid/ht/2.0);

    /* alpha is the angle the line is relative to horizontal */
    alpha = atan2(dy,-dx);

    /* ddx, ddy is amount to move end of line back so that arrowhead point
       ends where line used to */
    ddx = lpt*0.9 * cos(alpha);
    ddy = lpt*0.9 * sin(alpha);

    /* move endpoint of line back */
    mx = x2 + ddx;
    my = y2 + ddy;

    l = sqrt(dx * dx + dy * dy);
    sina = dy / l;
    cosa = dx / l;
    xb = mx * cosa - my * sina;
    yb = mx * sina + my * cosa;

    /* (xs,ys) are a point the length (height) of the arrowhead from
       the end of the shaft */
    xs =  (xb-ht) * cosa + yb * sina + .5;
    ys = -(xb-ht) * sina + yb * cosa + .5;

    /* lengthen the tail if type 2 */
    if (type == 2)
	x = xb - ht * 1.2;
    /* shorten the tail if type 3 */
    else if (type == 3)
	x = xb - ht * 0.8;
    else
	x = xb - ht;

    /* half the width of the arrowhead */
    y = yb - wid / 2;

    /* xc,yc is one point of arrowhead tail */
    xc =  x * cosa + y * sina + .5;
    yc = -x * sina + y * cosa + .5;

    /* the x component of the endpoint of the line */
    xxb = x2 * cosa - y2 * sina;

    /* xg,yg is one corner of the box enclosing the arrowhead */
    /* allow extra for a round line cap */
    xxb = xxb+objthick;

    xg =  xxb * cosa + y * sina + .5;
    yg = -xxb * sina + y * cosa + .5;

    y = yb + wid / 2;
    /* xd,yd is other point of arrowhead tail */
    xd =  x * cosa + y * sina + .5;
    yd = -x * sina + y * cosa + .5;

    /* xh,yh is the other corner of the box enclosing the arrowhead */
    /* allow extra for a round line cap */
    xh =  xxb * cosa + y * sina + .5;
    yh = -xxb * sina + y * cosa + .5;

    /* pass back these two corners to the caller */
    *c1x = xg;
    *c1y = yg;
    *c2x = xh;
    *c2y = yh;

    points[*npoints].x = xc; points[(*npoints)++].y = yc;
    points[*npoints].x = mx; points[(*npoints)++].y = my;
    points[*npoints].x = xd; points[(*npoints)++].y = yd;
    if (type != 0) {
	points[*npoints].x = xs; points[(*npoints)++].y = ys; /* add point on shaft */
	points[*npoints].x = xc; points[(*npoints)++].y = yc; /* connect back to first point */
    }
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
    double	x1, y1;
    double	x2, y2;
    int		*x, *y;
    int		direction;
    F_arrow	*arrow;
{
    double	r, alpha, beta, dy, dx;
    double	lpt,h;

    dy=y2-y1;
    dx=x2-x1;
    r=sqrt(dx*dx+dy*dy);
    h = (double) arrow->ht;
    /* lpt is the amount the arrowhead extends beyond the end of the line */
    lpt = arrow->thickness/2.0/(arrow->wid/h/2.0);
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
	    if (PSisomap[t->font+1] == FALSE) {
		fprintf(tfp, "/%s /%s-iso isovec ReEncode\n", PSFONT(t), PSFONT(t));
		PSisomap[t->font+1] = TRUE;
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
