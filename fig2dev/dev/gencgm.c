/*
 * Fig2dev: Translate Fig code to various Devices
 * Copyright (c) 1991 by Micah Beck
 * Parts Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2015 by Brian V. Smith
 * Parts Copyright (c) 2015-2023 by Thomas Loimer
 *
 * Any party obtaining a copy of these files is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and documentation
 * files (the "Software"), including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense and/or sell copies
 * of the Software, and to permit persons who receive copies from any such
 * party to do so, with the only requirement being that the above copyright
 * and this permission notice remain intact.
 *
 */


/*
 * gencgm.c: convert fig to clear text version-1 Computer Graphics Metafile
 *
 * Copyright (c) 1999 by Philippe Bekaert
 *	Computer Graphics Research Group, K.U.Leuven, Leuven, Belgium
 *	e-mail: Philippe.Bekaert@cs.kuleuven.ac.be
 *	www: http://www.cs.kuleuven.ac.be/~graphics/
 *
 * Limitations:
 *
 * - old style splines are not supported by this driver. New style (X) splines
 * are automatically converted to polylines and thus are supported.
 * - CGMv1 doesn't support bitmap images, so forget your picbox polylines.
 * - CGMv1 doesn't support the dash-triple-dotted linestyle. Such lines
 * will appear as solid lines.
 * - CGMv1 supports only 6 patterns which are different from FIG's 22
 * fill pattern.
 * - CGMv1 doesn't support line cap and join styles. The correct appearance
 * of arrows depends on the cap style the viewer uses (not known
 * at conversion time, but can be set using the -r driver option).
 * - a CGM file may look quite different when viewed with different
 * CGM capable drawing programs. This is especially so for text: the
 * text font e.g. needs to be recognized and supported by the viewer.
 * Same is true for special characters in text strings, text orientation, ...
 * This driver also assumes that the background remains visible behind
 * hatched polygons for correct appearance of pattern filled shapes with
 * non-white background. This is not always true ...
 *
 * Known bugs:
 *
 * - parts of objects close to arrows on wide curves might sometimes be hidden.
 * (They disappear together with part of the polyline covered by the arrow
 * that has to be erased in order to get a clean arrow tip). This problem
 * requires a different arrow drawing strategy.
 *
 * Notes:
 *
 * - not all CGM capable drawing programs can read clear-text CGM files.
 * You will sometimes need to convert to binary or character CGM encoding.
 * The RALCGM program, distributed in C source code form
 * at http://www.agocg.ac.uk:8080/CGM.html, will enable such conversion
 * as well as viewing under UNIX/X.
 *
 * History:
 *
 *  - July, 30 1998:   initial version.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "fig2dev.h"	/* includes bool.h and object.h */
//#include "object.h"
#include "messages.h"
#include "pi.h"

#define	UNDEFVALUE	-100	/* undefined attribute value */
#define	FILL_COLOR_INDEX 999	/* special color index for solid filled shapes.
				 * see fillshade() and conv_color(). */
#define	EPSILON		1e-4	/* small floating point value */

static	int rounded_arrows;	/* If rounded_arrows is false, the position
				 * of arrows will be corrected for
				 * compensating line width effects. This
				 * correction is not needed if arrows appear
				 * rounded with the used CGM viewer.
				 * See -r driver command line option. */

static	bool	 binary_output = false;	/* default is ASCII output */
static	FILE	*saveofile;	/* used when piping to ralcgm for binary output */
static	char	 *cgmcom;

static struct	_rgb {
	float r, g, b;
}
stdcols[NUM_STD_COLS] = {		/* standard colors */
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

/* CGM patterns are numbered 1-6 (I use 0 for nonexistant patterns) */

int map_pattern [22] = { 0, 0, 0, 4,
			 3, 6, 0, 0,
			 1, 2, 5, 0,
			 0, 0, 0, 0,
			 0, 0, 0, 0,
			 0, 0 };


void
gencgm_start(F_compound *objects)
{
	int	 i;
	char	*p, *figname;
	char	*figname_buf = NULL;

	if (from) {
		figname_buf = strdup(from);
		figname = figname_buf;
		p = strrchr(figname, '/');
		if (p)
			/* remove path from name for comment in file */
			figname = p+1;
		p = strchr(figname, '.');
		if (p)
			*p = '\0';		/* and extension */
	} else {
		figname = "(stdin)";
	}
	if (binary_output) {
		/* pipe output through "ralcgm -b" to produce binary CGM */
		/* but first close the output file that main() opened */
		saveofile = tfp;
		if (tfp != stdout)
			fclose(tfp);
		if (to) {
			cgmcom = malloc(strlen(to) + 13);
			strcpy(cgmcom, "ralcgm -b - ");
			strcat(cgmcom, to);
		} else {
			cgmcom = malloc((size_t) 14);
			strcpy(cgmcom, "ralcgm -b - -");
		}
		/* open pipe to ralcgm */
		if ((tfp = popen(cgmcom,"w")) == 0) {
			fprintf(stderr, "fig2dev: Can't open pipe to ralcgm, "
					"producing ASCII CGM instead.\n");
			fprintf(stderr, "Command was: %s\n", cgmcom);
			/* failed, revert back to ASCII */
			tfp = saveofile;
			free(cgmcom);
			binary_output = false;
		}
	}

	fprintf(tfp, "BEGMF '%s';\n", figname);
	fprintf(tfp, "mfversion 1;\n");
	fprintf(tfp, "mfdesc 'Converted from %s using fig2dev -Lcgm';\n",
			from? from : "(stdin)");
	fprintf(tfp, "mfelemlist 'DRAWINGPLUS';\n");
	fprintf(tfp, "vdctype integer;\n");

#define LATEX_FONT_BASE 2	/* index of first LaTeX-like text font */
#define NUM_LATEX_FONTS 5	/* number of LaTeX like text fonts */
#define PS_FONT_BASE 7		/* index of first PostScript text font */
#define NUM_PS_FONTS 35		/* number of PostScript fonts */
	fprintf(tfp, "fontlist 'Hardware',\n\
  'Times New Roman', 'Times New Roman Bold', 'Times New Roman Italic',\n\
  'Helvetica', 'Courier',\n\
  'Times-Roman', 'Times-Italic',\n\
  'Times-Bold', 'Times-BoldItalic',\n\
  'AvantGarde-Book', 'AvantGarde-BookOblique',\n\
  'AvantGarde-Demi', 'AvantGarde-DemiOblique',\n\
  'Bookman-light', 'Bookman-lightItalic',\n\
  'Bookman-Demi', 'Bookman-DemiItalic',\n\
  'Courier', 'Courier-Oblique',\n\
  'Courier-Bold', 'Courier-BoldOblique',\n\
  'Helvetica', 'Helvetica-Oblique',\n\
  'Helvetica-Bold', 'Helvetica-BoldOblique',\n\
  'Helvetica-Narrow', 'Helvetica-Narrow-Oblique',\n\
  'Helvetica-Narrow-Bold', 'Helvetica-Narrow-BoldOblique',\n\
  'NewCenturySchlbk-Roman', 'NewCenturySchlbk-Italic',\n\
  'NewCenturySchlbk-Bold', 'NewCenturySchlbk-BoldItalic',\n\
  'Palatino-Roman', 'Palatino-Italic',\n\
  'Palatino-Bold', 'Palatino-BoldItalic',\n\
  'Symbol', 'ZapfChancery-MediumItalic', 'ZapfDingbats';\n");

	fprintf(tfp, "BEGMFDEFAULTS;\n");
	fprintf(tfp, "  vdcext (0,0) (%d,%d);\n", urx-llx, ury-lly);
	/* see _pos() below */
	fprintf(tfp, "  clip off;\n");
	fprintf(tfp, "  colrmode indexed;\n");
	fprintf(tfp, "  colrtable 1");	/* color table */
	for (i=0; i<NUM_STD_COLS; i++) {	/* standard colors */
		fprintf(tfp, "\n	%d %d %d",
				(int)(stdcols[i].r * 255.),
				(int)(stdcols[i].g * 255.),
				(int)(stdcols[i].b * 255.));
	}
	for (i=0; i<num_usr_cols; i++) {	/* user defined colors */
		fprintf(tfp, "\n	%d %d %d",
				user_colors[i].r,
				user_colors[i].g,
				user_colors[i].b);
	}
	fprintf(tfp, ";\n");
	fprintf(tfp, "  linewidthmode abs;\n");
	fprintf(tfp, "  edgewidthmode abs;\n");
	fprintf(tfp, "  backcolr 255 255 255;\n");
	fprintf(tfp, "  textprec stroke;\n");
	fprintf(tfp, "  transparency ON;\n");
	fprintf(tfp, "ENDMFDEFAULTS;\n");

	fprintf(tfp, "BEGPIC '%s';\n", figname);
	fprintf(tfp, "BEGPICBODY;\n");

	/* print any whole-figure comments prefixed with "%" */
	if (objects->comments) {
		fprintf(tfp,"%% %%\n");
		print_comments("% ",objects->comments, " %");
		fprintf(tfp,"%% %%\n");
	}
	if (figname_buf)
		free(figname_buf);
}

int
gencgm_end(void)
{
	fprintf(tfp,"%% End of Picture %%\n");
	fprintf(tfp, "ENDPIC;\n");
	fprintf(tfp, "ENDMF;\n");

	/* if user wanted binary, close pipe to ralcgm */
	if (binary_output) {
		if (pclose(tfp) != 0) {
			fprintf(stderr,"Error in ralcgm command\n");
			fprintf(stderr,"command was: %s\n", cgmcom);
			free(cgmcom);
			return -1;
		}
		free(cgmcom);
	}
	/* we've already closed the original output file */
	tfp = 0;

	/* all ok */
	return 0;
}

void
gencgm_option(char opt, char *optarg)
{
	(void)	optarg;

	rounded_arrows = false;
	switch (opt) {
	case 'a':
		binary_output = true;	/* call ralcgm to convert output to
					   binary cgm */
		break;

	case 'r':
		rounded_arrows = true;
		break;

	case 'G':
	case 'L':
		break;

	default:
		/* other CGM driver options to consider are:
		 * faithful reproduction of FIG linestyles and fill patterns
		 * (linetyles e.g. by drawing multiple short lines), other
		 * CGM encodings beside clear text, non-white (e.g. black)
		 * background with corresponding change of foreground color...*/
		put_msg(Err_badarg, opt, "cgm");
		exit(1);
	}
}

/* Coordinates are translated such that the lower left corner has
 * coordinates (0,0). We use fig units for spacing (no coordinate scaling)
 * that means: 1200 units/inch. */

static void
_pos(int x, int y)
{
	fprintf(tfp, "(%d,%d)", x-llx, ury-y);
}

/* only reverses y if Y axis points down (relative position) */
static void
_relpos(int x, int y)
{
	fprintf(tfp, "(%d,%d)", x, -y);
}

static void
point(F_point *p)
{
	_pos(p->x, p->y);
}

static void
pos(F_pos *p)
{
	_pos(p->x, p->y);
}

/* piece of code to avoid unnecessary attribute changes */
#define chkcache(val, cachedval)	\
	if (val == cachedval)		\
		return;			\
	else				\
		cachedval = val;

/* Convert FIG line style to CGM line style. CGM knows 5 styles
 * with fortunately corresond to the first 5 FIG line styles. The triple
 * dotted FIG line style is reproduced as a solid line. */

static int
conv_linetype(int type)
{
	if (type < 0)
		type = -1;
	else if (type > 4)
		type %= 5;
	return type;
}

static void
linetype(int type)
{
	static int oldtype = UNDEFVALUE;

	chkcache(type, oldtype);
	type = conv_linetype(type);
	fprintf(tfp, "linetype %d;\n", type+1);
}

static void
edgetype(int type)
{
	static int oldtype = UNDEFVALUE;

	chkcache(type, oldtype);
	type = conv_linetype(type);
	fprintf(tfp, "edgetype %d;\n", type+1);
}

static void
linewidth(int width)
{
	static int oldwidth = UNDEFVALUE;

	chkcache(width, oldwidth);
	fprintf(tfp, "linewidth %d;\n", width);
}

static void
edgewidth(int width)
{
	static int oldwidth = UNDEFVALUE;

	chkcache(width, oldwidth);
	fprintf(tfp, "edgewidth %d;\n", width);
}

/* Converts FIG color index to CGM color index into the color table
 * defined in the pre-amble (see gencgm_start(). */

static int
conv_color(int color)
{
	if (color < 0)			/* default color = black */
		color = 0;
	else if (color == FILL_COLOR_INDEX) /* special index used for solid */
		/* fill color, see fillshade() */
		color = NUM_STD_COLS + num_usr_cols;

	return color+1;	/* CGM counts colors starting from 1 instead of 0 */
}

static void
linecolr(int color)
{
	static int oldcolor = UNDEFVALUE;

	chkcache(color, oldcolor);
	color = conv_color(color);
	fprintf(tfp, "linecolr %d;\n", color);
}

static void
edgecolr(int color)
{
	static int oldcolor = UNDEFVALUE;

	chkcache(color, oldcolor);
	color = conv_color(color);
	fprintf(tfp, "edgecolr %d;\n", color);
}

static void
edgevis(int onoff)
{
	static int old = UNDEFVALUE;

	chkcache(onoff, old);
	fprintf(tfp, "edgevis %s;\n", onoff ? "ON" : "OFF");
}

static void
lineattr(int type, int width, int color)
{
	linetype(type);
	linewidth(width);
	linecolr(color);
}

static void
edgeattr(int vis, int type, int width, int color)
{
	edgevis(vis);
	edgetype(type);
	edgewidth(width);
	edgecolr(color);
}

/* sets CGM interior style */

typedef enum {SOLID, HOLLOW, HATCH, EMPTY, UNDEF} INTSTYLE;

static void
intstyle(INTSTYLE style)
{
	static INTSTYLE oldstyle = UNDEF;

	chkcache(style, oldstyle);

	switch (style) {
	case HOLLOW:
		fprintf(tfp, "intstyle HOLLOW;\n");
		break;
	case SOLID:
		fprintf(tfp, "intstyle SOLID;\n");
		break;
	case HATCH:
		fprintf(tfp, "intstyle HATCH;\n");
		break;
	case EMPTY:
		fprintf(tfp, "intstyle EMPTY;\n");
		break;
	default:
		fprintf(stderr, "Unrecognized intstyle %d (program error).\n",
				style); }
}

static int oldfillcolor = UNDEFVALUE;

/* updates unconditionally */

static void
_fillcolr(int color)
{
	oldfillcolor = color;
	color = conv_color(color);
	fprintf(tfp, "fillcolr %d;\n", color);
}

/* set fill color if standard or user defined color */

static void
fillcolr(int color)
{
	chkcache(color, oldfillcolor);
	_fillcolr(color);
}

/* used to set solid fill color if something else than standard or user defined
 * color, see fillshade(). */

static void fillcolrgb(int r, int g, int b)
{
	static int oldrgb = UNDEFVALUE;
	int rgb = (r * 256 + g) * 256 + b;
	if (rgb != oldrgb) {
		oldrgb = rgb;
		fprintf(tfp, "colrtable %d %d %d %d;\n",
				conv_color(FILL_COLOR_INDEX), r, g, b);
		_fillcolr(FILL_COLOR_INDEX);
	} else
		fillcolr(FILL_COLOR_INDEX);
}


/* Converts hatch pattern index. FIG knows 16 patterns, CGM only 6
 * different ones. */

static int conv_pattern_index(int index)
{
	return map_pattern[index];
}

static void
hatchindex(int index)
{
	static int oldindex = UNDEFVALUE;

	chkcache(index, oldindex);
	index = conv_pattern_index(index);
	fprintf(tfp, "hatchindex %d;\n", index);
}

/* Looks up RGB color values for color with given index. */

static void
getrgb(int color, int *r, int *g, int *b)
{
	if (color < 0)	/* DEFAULT color is black */
		color = 0;
	if (color < NUM_STD_COLS) {
		*r = stdcols[color].r * 255.;
		*g = stdcols[color].g * 255.;
		*b = stdcols[color].b * 255.;
	} else {
		int i;
		for (i=0; i<num_usr_cols; i++) {
			if (color == user_col_indx[i])
				break;
		}
		if (i < num_usr_cols) {
			*r = user_colors[i].r;
			*g = user_colors[i].g;
			*b = user_colors[i].b;
		} else {
			fprintf(stderr,
					"getrgb: color %d is undefined "
					"(program error).\n", color);
		}
	}
}

/* Computes and sets fill color for solid filled shapes (fill style 0 to 40). */

static void
fillshade(F_line *l)
{
	int shade, r, g, b;
	float f;

	switch (l->fill_color) {
	case -1:			/* default or black fill color */
	case 0:
		shade = round((float)(20 - l->fill_style) * 255. / 20.);
		fillcolrgb(shade, shade, shade);
		break;
	case 7:			/* white fill color */
		shade = round((float)l->fill_style * 255. / 20.);
		fillcolrgb(shade, shade, shade);
		break;
	default:		/* compute shade as a mix of black/white */
		if (l->fill_style == 0) {	/* with fill color */
			fillcolr(0);
		} else if (l->fill_style < 20) {
			getrgb(l->fill_color, &r, &g, &b);
			f = (float)l->fill_style / 20.;
			fillcolrgb(round(r * f), round(g * f), round(b * f));
		} else if (l->fill_style == 20) {
			fillcolr(l->fill_color);
		} else if (l->fill_style < 40) {
			getrgb(l->fill_color, &r, &g, &b);
			f = (float)(l->fill_style - 20) / 20.;
			fillcolrgb(round(r + f*(255-r)), round(g + f*(255-g)),
					round(b + f*(255-b)));
		} else if (l->fill_style == 40) {
			fillcolr(7);
		}
	}
}

/* Draws interior and outline of a simple closed shape. Cannot be
 * used for polylines and arcs (open shapes) or for arcboxes
 * (closed but not a simple CGM primitive). */

static void
shape(F_line *l, void (*drawshape)(F_line *))
{
	int style = l->fill_style;

	if (style < 0) {			/* unfilled shape */
		intstyle(EMPTY);
		edgeattr(1, l->style, l->thickness, l->pen_color);
		drawshape(l);
		return;
	}

	intstyle(SOLID);
	if (style >= 0 && style <= 40) {	/* solid filled shape */
		fillshade(l);
		edgeattr(1, l->style, l->thickness, l->pen_color);
	} else {			/* pattern filled shape: draw a */
		fillcolr(l->fill_color);/* first time for correct pattern */
		edgevis(0);		/* background color. */
	}
	drawshape(l);

	if (style > 40) {		/* pattern filled shape */
		intstyle(HATCH);
		hatchindex(style-41);
		fillcolr(l->pen_color);
		edgeattr(1, l->style, l->thickness, l->pen_color);
		drawshape(l);
	}
}

/* Draws interior of a closed shape, taking into account fill color
 * and pattern. Boundary must be drawn separately. Used for
 * polylines, arcboxes and arcs. */

static void
shape_interior(F_line *l, void (*drawshape)(F_line *))
{
	int style = l->fill_style;
	if (style < 0)
		return;

	edgevis(0);			/* don't draw edges */

	intstyle(SOLID);
	if (style >= 0 && style <= 40)	/* solid filled shape */
		fillshade(l);
	else				/* fill pattern background */
		fillcolr(l->fill_color);
	drawshape(l);

	if (style > 40) {		/* pattern filled shape */
		intstyle(HATCH);
		hatchindex(style-41);
		fillcolr(l->pen_color);
		drawshape(l);
	}
}

typedef struct Dir {double x, y;} Dir;
#define ARROW_INDENT_DIST	0.8
#define ARROW_POINT_DIST	1.2

/* Arrows appear this much longer with projected line caps. */

#define ARROW_EXTRA_LEN(a)	((double)a->ht / (double)a->wid * a->thickness)

/* Draws an arrow ending at point P pointing into direction dir.
 * Type and attributes as required by a and l. This routine works
 * for both lines and arc (in that case l should be a F_arc *). */

static void
cgm_arrow(F_point *P, F_arrow *a, F_line *l, Dir *dir)
{
	F_point s1, s2, t, p;

	p = *P;
	if (!rounded_arrows) {
		/* move the arrow backwards in order to let it end at the
		   correct spot */
		double f = ARROW_EXTRA_LEN(a);
		p.x = round(p.x - f * dir->x);
		p.y = round(p.y - f * dir->y);
	}
	s1.x = round(p.x - a->ht * dir->x + a->wid*0.5 * dir->y);
	s1.y = round(p.y - a->ht * dir->y - a->wid*0.5 * dir->x);
	s2.x = round(p.x - a->ht * dir->x - a->wid*0.5 * dir->y);
	s2.y = round(p.y - a->ht * dir->y + a->wid*0.5 * dir->x);
	t = p;

	if (a->type == 0) {		/* old style stick arrow */
		lineattr(0, (int)a->thickness, l->pen_color);
	} else {			/* polygonal arrows */
		intstyle(SOLID);
		switch (a->style) {
		case 0:
			fillcolr(7);
			break;
		case 1:
			fillcolr(l->pen_color);
			break;
		default:
			fprintf(stderr, "Unsupported FIG arrow style %d !!\n",
					a->style);
		}
		edgeattr(1, 0, (int)a->thickness, l->pen_color);
	}

	switch (a->type) {
	case 0:				/* stick type */
		fprintf(tfp, "line "); point(&s1); point(&p); point(&s2);
		fprintf(tfp, ";\n");
		break;
	case 1:				/* closed triangle */
		fprintf(tfp, "polygon "); point(&s1); point(&p); point(&s2);
		fprintf(tfp, ";\n");
		break;
	case 2:				/* indented hat */
		t.x = round(p.x - a->ht*ARROW_INDENT_DIST * dir->x);
		t.y = round(p.y - a->ht*ARROW_INDENT_DIST * dir->y);
		fprintf(tfp, "polygon ");
		point(&s1); point(&p); point(&s2); point(&t);
		fprintf(tfp, ";\n");
		break;
	case 3:				/* pointed hat */
		t.x = round(p.x - a->ht*ARROW_POINT_DIST * dir->x);
		t.y = round(p.y - a->ht*ARROW_POINT_DIST * dir->y);
		fprintf(tfp, "polygon ");
		point(&s1); point(&p); point(&s2); point(&t);
		fprintf(tfp, ";\n");
		break;
	default:
		fprintf(stderr, "Unsupported FIG arrow type %d.\n", a->type);
	}
}

/* Returns length of the arrow. Used to shorten lines/arcs at
 * an end where an arrow needs to be drawn. */

static double
arrow_length(F_arrow *a)
{
	double len;

	switch (a->type) {
	case 0:				/* stick type */
		len = ARROW_EXTRA_LEN(a);
		break;
	case 1:				/* closed triangle */
		len =  a->ht;
		break;
	case 2:				/* indented hat */
		len =  ARROW_INDENT_DIST * a->ht;
		break;
	case 3:				/* pointed hat */
		len =  ARROW_POINT_DIST * a->ht;
		break;
	default:
		len =  0.;
	}
	if (rounded_arrows)
		len -= ARROW_EXTRA_LEN(a);

	return len;
}

/* Computes distance and normalized direction vector from q to p.
 * Returns true if the points do not coincide and false if they do. */

static int
direction(F_point *p, F_point *q, Dir *dir, double *dist)
{
	dir->x = p->x - q->x;
	dir->y = p->y - q->y;
	*dist = sqrt((dir->x) * (dir->x) + (dir->y) * (dir->y));
	if (*dist < EPSILON)
		return false;
	dir->x /= *dist;
	dir->y /= *dist;
	return true;
}

/* Replaces P by the starting point of the arrow pointing from q to P.
 * Returns true if the arrow is shorter than the line segment [qP]. In
 * this case, we shorten the polyline in order to achieve a better fit with
 * the arrow. If the arrow is longer than the line segment, shortening
 * the segment wont help, and the polyline needs to be erased under the
 * arrow after drawing the polyline and before drawing the arrow. */

static int
polyline_arrow_adjust(F_point *P, F_point *q, F_arrow *a)
{
	double D, L;
	Dir dir;

	if (direction(P, q, &dir, &D)) {
		L = arrow_length(a);
		P->x = round(P->x - L * dir.x);
		P->y = round(P->y - L * dir.y);
		return (L < D);
	}
	fprintf(stderr,"Warning: arrow at zero-length line segment omitted.\n");
	return true;
}

static void
_line(int x1, int y1, int x2, int y2)
{
	fprintf(tfp, "line "); _pos(x1, y1); fprintf(tfp, " "); _pos(x2, y2);
	fprintf(tfp, ";\n");
}

static void
line(F_point *p, F_point *q)
{
	_line(p->x, p->y, q->x, q->y);
}

/* draws polyline boundary (with arrows if needed) */

static void
polyline(F_line *l)
{
	F_point *p, *q, P0, Pn;
	int count, erase_head=false, erase_tail=false;
	Dir dir;
	double d;

	if (!l->points) return;
	if (!l->points->next) {
		fprintf(tfp, "line "); point(l->points); point(l->points);
		fprintf(tfp, ";\n");
		if (l->for_arrow || l->back_arrow)
			fprintf(stderr, "Warning: arrow at zero-length line "
					"segment omitted.\n");
		return;
	}		/* at least two different points now */

	fprintf(tfp, "line");
	for (q=p=l->points, count=0; p; q=p, p=p->next) {
		if (count!=0 && count%5 == 0)
			fprintf(tfp, "\n	  ");
		fprintf(tfp, " ");

		if (count == 0 && l->back_arrow) {  /* first point with arrow */
			P0 = *p;
			q = p->next;
			erase_head = !polyline_arrow_adjust(&P0, q,
					l->back_arrow);
			point(erase_head ? p : &P0);
		}
		else if (!p->next && l->for_arrow) { /* last point with arrow */
			Pn = *p;
			/* q is one but last point */
			erase_tail = !polyline_arrow_adjust(&Pn, q,
					l->for_arrow);
			point(erase_tail ? p : &Pn);
		}
		else					/* normal point */
			point(p);

		count++;
	}
	fprintf(tfp, ";\n");

	if (l->back_arrow) {
		p = l->points;
		q = l->points->next;
		if (direction(p, q, &dir, &d)) {
			if (erase_head) {
				F_point Q;
				Q.x = round(p->x + dir.x * l->thickness/2.);
				Q.y = round(p->y + dir.y * l->thickness/2.);
				lineattr(0, l->thickness*3/2, 7);
				line(&Q, &P0);
			}
			cgm_arrow(p, l->back_arrow, l, &dir);
		}
	}

	if (l->for_arrow) {
		for (q=l->points, p=l->points->next; p->next; q=p, p=p->next) {}
		/* p is the last point, q is the one but last point */
		if (direction(p, q, &dir, &d)) {
			if (erase_tail) {
				F_point Q;
				Q.x = round(p->x + dir.x * l->thickness/2.);
				Q.y = round(p->y + dir.y * l->thickness/2.);
				lineattr(0, l->thickness*3/2, 7);
				line(&Pn, &Q);
			}
			cgm_arrow(p, l->for_arrow, l, &dir);
		}
	}
}

static void
rect(F_line *l)
{
	if (!l->points || !l->points->next || !l->points->next->next) {
		fprintf(stderr, "Warning: Invalid FIG box omitted.\n");
		return;
	}

	fprintf(tfp, "rect ");
	point(l->points); fprintf(tfp, " ");
	point(l->points->next->next); fprintf(tfp, ";\n");
}

static void
polygon(F_line *l)
{
	F_point *p;
	int count;

	fprintf(tfp, "polygon");
	for (p=l->points, count=0; p; p=p->next) {
		if (count!=0 && count%5 == 0)
			fprintf(tfp, "\n	  ");
		fprintf(tfp, " "); point(p);
		count++;
	}
	fprintf(tfp, ";\n");
}

static void
arcboxsetup(F_line *l, int *llx, int *lly, int *urx, int *ury, int *r)
{
	*llx = l->points->x;
	*lly = l->points->y;
	*urx = l->points->next->next->x;
	*ury = l->points->next->next->y;
	*r = l->radius;
	if (*llx > *urx) {int t = *urx; *urx = *llx; *llx = t;}
	if (*lly > *ury) {int t = *ury; *ury = *lly; *lly = t;}
}

static void
_arcctr(int cx, int cy, int x1, int y1, int x2, int y2, int r)
{
	fprintf(tfp, "arcctr ");
	_pos(cx, cy);
	fprintf(tfp, " ");
	_relpos(x2, y2);
	fprintf(tfp, " ");
	_relpos(x1, y1);
	fprintf(tfp, " %d;\n", r);
}

static void
arcboxoutline(F_line *l)
{
	int llx, lly, urx, ury, r;
	arcboxsetup(l, &llx, &lly, &urx, &ury, &r);

	_line(llx, lly+r, llx, ury-r);
	_line(llx+r, ury, urx-r, ury);
	_line(urx, ury-r, urx, lly+r);
	_line(urx-r, lly, llx+r, lly);

	_arcctr(llx+r, ury-r, 0, r, -r, 0, r);
	_arcctr(urx-r, ury-r, r, 0, 0, r, r);
	_arcctr(urx-r, lly+r, 0, -r, r, 0, r);
	_arcctr(llx+r, lly+r, -r, 0, 0, -r, r);
}

static void
_circle(int cx, int cy, int r)
{
	fprintf(tfp, "circle ");
	_pos(cx, cy);
	fprintf(tfp, " %d;\n", r);
}

static void
arcboxinterior(F_line *l)
{
	int llx, lly, urx, ury, r;
	arcboxsetup(l, &llx, &lly, &urx, &ury, &r);

	fprintf(tfp, "polygon ");
	_pos(llx  , lly+r); fprintf(tfp, " ");
	_pos(llx  , ury-r); fprintf(tfp, " ");
	_pos(llx+r, ury-r); fprintf(tfp, " ");
	_pos(llx+r, ury  ); fprintf(tfp, "\n	  ");
	_pos(urx-r, ury  ); fprintf(tfp, " ");
	_pos(urx-r, ury-r); fprintf(tfp, " ");
	_pos(urx  , ury-r); fprintf(tfp, " ");
	_pos(urx  , lly+r); fprintf(tfp, "\n	  ");
	_pos(urx-r, lly+r); fprintf(tfp, " ");
	_pos(urx-r, lly  ); fprintf(tfp, " ");
	_pos(llx+r, lly  ); fprintf(tfp, " ");
	_pos(llx+r, lly+r); fprintf(tfp, ";\n");

	_circle(llx+r, ury-r, r);
	_circle(urx-r, ury-r, r);
	_circle(urx-r, lly+r, r);
	_circle(llx+r, lly+r, r);
}

static void
picbox(F_line *l)
{
	static int wgiv = 0;
	if (!wgiv) {
		fprintf(stderr, "Warning: Pictures not supported in CGM "
				"language\n");
		wgiv = 1;
	}
	polyline(l);
}

void
gencgm_line(F_line *l)
{
	/* print any comments prefixed with "%" */
	print_comments("% ",l->comments, " %");

	switch (l->type) {
	case T_POLYLINE:
		fprintf(tfp,"%% Polyline %%\n");
		shape_interior(l, polygon);		/* draw interior */
		lineattr(l->style, l->thickness, l->pen_color);
		polyline(l);			/* draw boundary */
		break;
	case T_BOX:
		fprintf(tfp,"%% Box %%\n");
		shape(l, rect);			/* simple closed shape */
		break;
	case T_POLYGON:
		fprintf(tfp,"%% Polygon %%\n");
		shape(l, polygon);
		break;
	case T_ARC_BOX:
		fprintf(tfp,"%% Arc Box %%\n");
		shape_interior(l, arcboxinterior);
		lineattr(l->style, l->thickness, l->pen_color);
		arcboxoutline(l);
		break;
	case T_PIC_BOX:
		fprintf(tfp,"%% Imported Picture %%\n");
		picbox(l);
		break;
	default:
		fprintf(stderr, "Unsupported FIG polyline type %d.\n", l->type);
		return;
	}
}

void
gencgm_spline(F_spline *s)
{
	static int wgiv = 0;

	print_comments("% ", s->comments, "");
	if (!wgiv) {
		fputs("Warning: the CGM driver doesn't support (old style) "
				"FIG splines.\n"
			"Suggestion: convert your (old?) FIG image by loading "
				"it into xfig v3.2\n"
			"or higher and saving again.\n", stderr);
		wgiv = 1;
	}
}

static void
circle(F_ellipse *e)
{
	_circle(e->center.x, e->center.y, e->radiuses.x);
}

/* rotates counter clockwise around origin */

static void
rotate(double *x, double *y, double angle)
{
	double s = sin(angle), c = cos(angle), xt = *x, yt = *y;
	*x =	c * xt + s * yt;
	*y = -s * xt + c * yt;
}

static void
ellipsetup(F_ellipse *e, double *x1, double *y1, double *x2, double *y2)
{
	double r1 = e->radiuses.x, r2 = e->radiuses.y;
	*x1 = r1; *y1 = 0 ;
	*x2 = 0 ; *y2 = r2;
	rotate(x1, y1, e->angle);
	*x1 += e->center.x;
	*y1 += e->center.y;
	rotate(x2, y2, e->angle);
	*x2 += e->center.x;
	*y2 += e->center.y;
}

static void
_ellipse(int cx, int cy, int x1, int y1, int x2, int y2)
{
	fprintf(tfp, "ellipse ");
	_pos(cx, cy); fprintf(tfp, " ");
	_pos(x1, y1); fprintf(tfp, " ");
	_pos(x2, y2); fprintf(tfp, ";\n");
}

static void
ellipse(F_ellipse *e)
{
	double x1, y1, x2, y2;
	ellipsetup(e, &x1, &y1, &x2, &y2);
	_ellipse(e->center.x, e->center.y, (int)x1, (int)y1, (int)x2, (int)y2);
}

void
gencgm_ellipse(F_ellipse *e)
{
	/* print any comments prefixed with "%" */
	print_comments("% ",e->comments, " %");

	switch (e->type) {
	case T_ELLIPSE_BY_RAD:
	case T_ELLIPSE_BY_DIA:
		fprintf(tfp,"%% Ellipse %%\n");
		shape((F_line *)e, (void (*)(F_line *))ellipse);
		break;
	case T_CIRCLE_BY_RAD:
	case T_CIRCLE_BY_DIA:
		fprintf(tfp,"%% Circle %%\n");
		shape((F_line *)e, (void (*)(F_line *))circle);
		break;
	default:
		fprintf(stderr, "Unsupported FIG ellipse type %d.\n", e->type);
	}
}

static char *
arctype(int type)
{
	switch (type) {
	case T_OPEN_ARC:
		return "CHORD";
	case T_PIE_WEDGE_ARC:
		return "PIE";
	default:
		fprintf(stderr, "Unsupported FIG arc type %d.\n", type);
	}
	return "UNKNOWN";
}

static void
arcinterior(F_arc *a)
{
	fprintf(tfp, "arc3ptclose ");
	pos(&a->point[0]); fprintf(tfp, " ");
	pos(&a->point[1]); fprintf(tfp, " ");
	pos(&a->point[2]); fprintf(tfp, " %s;\n", arctype(a->type));
}

/* integer cross product */

static int
icprod(int x1, int y1, int x2, int y2)
{
	return x1 * y2 - y1 * x2;
}

/* Returns true if the arc is a clockwise arc. */
static int
cwarc(F_arc *a)
{
	int x1 = a->point[1].x - a->point[0].x,
	    y1 = a->point[1].y - a->point[0].y,
	    x2 = a->point[2].x - a->point[1].x,
	    y2 = a->point[2].y - a->point[1].y;

	return (icprod(x1,y1,x2,y2) > 0);
}

/* reverses arc direction by swapping endpoints and arrows */

static void
arc_reverse(F_arc *arc)
{
	F_arrow *arw;
	F_pos pp;
	pp = arc->point[0]; arc->point[0] = arc->point[2]; arc->point[2] = pp;
	arw = arc->for_arrow;
	arc->for_arrow = arc->back_arrow;
	arc->back_arrow = arw;
}

static double
distance(double x1, double y1, double x2, double y2)
{
	double dx = x2-x1, dy=y2-y1;
	return sqrt(dx*dx + dy*dy);
}

static double
arc_radius(F_arc *a)
{
	return (distance((double)a->point[0].x, (double)a->point[0].y,
				a->center.x, a->center.y) +
			distance((double)a->point[1].x, (double)a->point[1].y,
				a->center.x, a->center.y) +
			distance((double)a->point[2].x, (double)a->point[2].y,
				a->center.x, a->center.y)) / 3.;
}

/* copies coordinates of pos p to point P */

static void
pos2point(F_point *P, F_pos *p)
{
	P->x = p->x; P->y = p->y;
}

static void
translate(double *x, double *y, double tx, double ty)
{
	*x += tx; *y += ty;
}

/* rotates the point P counter clockwise along the arc with center c and
 * radius R. */

static void
arc_rotate(F_point *P, double cx, double cy, double angle)
{
	double x = P->x, y = P->y;
	translate(&x, &y, -cx, -cy);
	rotate(&x, &y, angle);
	translate(&x, &y, +cx, +cy);
	P->x = round(x); P->y = round(y);
}

/* Replaces p by the starting point of the arc arrow ending at p. */

static void
arc_arrow_adjust(F_point *p, double cx, double cy, double R,
		F_arrow *arw, double dir)
{
	double L;
	if (!arw) return;

	L = arw->type != 0 ? arrow_length(arw) : arw->ht;
	arc_rotate(p, cx, cy, dir * M_PI * L / (2. * R + L));
}

/* computes midpoint of p1 and p2 on arc. */

static void
arc_midpoint(F_point *mid, F_point *p1, F_point *p2,
		double cx, double cy, double R)
{
	Dir dir; double d;
	direction(p1, p2, &dir, &d);
	*mid = *p1;
	arc_rotate(mid, cx, cy, M_PI * d / (2. * R + d) / 2.);
}

/* draws arc arrow ending at p and starting at q. */

static void
arc_arrow(F_pos *p, F_point *q, F_arrow *arw, F_arc *arc)
{
	F_point P;
	Dir dir; double d;

	if (!arw) return;

	pos2point(&P, p);
	direction(&P, q, &dir, &d);
	cgm_arrow(&P, arw, (F_line *)arc, &dir);
	if (arw->type == 0) {
		/* draw middle leg of old-style stick arrow */
		double f = arrow_length(arw);
		P.x = round(P.x - f * dir.x);
		P.y = round(P.y - f * dir.y);
		line(q, &P);
	}
}

static void
arcwitharrows(F_arc *a)
{
	F_point P0, P1, P2;
	double R = arc_radius(a);

	pos2point(&P0, &a->point[0]);
	arc_arrow_adjust(&P0, a->center.x, a->center.y, R, a->back_arrow, +1.);

	pos2point(&P2, &a->point[2]);
	arc_arrow_adjust(&P2, a->center.x, a->center.y, R, a->for_arrow, -1.);

	/* make sure P1 lays between P0 and P2 */
	arc_midpoint(&P1, &P0, &P2, a->center.x, a->center.y, R);

	fprintf(tfp, "arc3pt ");
	point(&P0); fprintf(tfp, " ");
	point(&P1); fprintf(tfp, " ");
	point(&P2); fprintf(tfp, ";\n");

	arc_arrow(&a->point[0], &P0, a->back_arrow, a);
	arc_arrow(&a->point[2], &P2, a->for_arrow, a);
}

static void
arcoutline(F_arc *a)
{
	if ((a->type == T_OPEN_ARC) && (a->thickness != 0) &&
			(a->back_arrow || a->for_arrow)) {
		arcwitharrows(a);
		return;
	}

	fprintf(tfp, "arc3pt ");
	pos(&a->point[0]); fprintf(tfp, " ");
	pos(&a->point[1]); fprintf(tfp, " ");
	pos(&a->point[2]); fprintf(tfp, ";\n");

	switch (a->type) {
	case T_PIE_WEDGE_ARC:
		fprintf(tfp, "line ");	/* close the pie wedge */
		pos(&a->point[2]); fprintf(tfp, " ");
		_pos(round(a->center.x), round(a->center.y)); fprintf(tfp, " ");
		pos(&a->point[0]); fprintf(tfp, ";\n");
		break;
	case T_OPEN_ARC:
	default:
		/* don't draw anything more than the arc itself. */
		break;
	}
}

void
gencgm_arc(F_arc *_a)
{
	F_arc a = *_a;

	/* print any comments prefixed with "%" */
	print_comments("% ",a.comments, " %");

	fprintf(tfp,"%% Arc %%\n");
	if (cwarc(&a))
		arc_reverse(&a);	/* make counter clockwise arc */

	shape_interior((F_line *)&a, (void (*)(F_line *))arcinterior);
	if (a.thickness > 0) {
		lineattr(a.style, a.thickness, a.pen_color);
		arcoutline(&a);
	}
}

static void
texttype(int type)
{
	static int oldtype = UNDEFVALUE;
	chkcache(type, oldtype);
	switch (type) {
	case T_LEFT_JUSTIFIED:
		fprintf(tfp, "textalign left base 0.0 0.0;\n");
		break;
	case T_CENTER_JUSTIFIED:
		fprintf(tfp, "textalign ctr base 0.0 0.0;\n");
		break;
	case T_RIGHT_JUSTIFIED:
		fprintf(tfp, "textalign right base 0.0 0.0;\n");
		break;
	default:
		fprintf(stderr, "Unsupported FIG text type %d.\n", type);
	}
}

/* Converts FIG font index to CGM font index into font table in the pre-amble
 * (see gencgm_start()) taking into account the flags. */

static int
conv_fontindex(int font, int flags)
{
	if (flags&4) {		/* postscript fonts */
		if (font < 0) font = 0;
		if (font >= NUM_PS_FONTS) {
			fprintf(stderr,
				"Unsupported FIG PostScript font index %d.\n",
				font);
			font =  1;			/* CGM hardware font */
		}
		else
			font = PS_FONT_BASE + font;
	} else {			/* LaTeX fonts */
		if (font <= 0) font = 1;	/* default font is Roman */
		if (font >= NUM_LATEX_FONTS) {
			fprintf(stderr,"Unsupported FIG LaTeX font index %d.\n",
				       font);
			font = 1;			/* CGM hardware font */
		}
		else
			font = LATEX_FONT_BASE + font - 1;
	}
	return font;
}

static void
textfont(int font, int flags)
{
	static int oldfont = UNDEFVALUE;
	font = conv_fontindex(font, flags);	/* first convert it ... */
	chkcache(font, oldfont);
	fprintf(tfp, "textfontindex %d;\n", font);
}

static void
textcolr(int color)
{
	static int oldcolor = UNDEFVALUE;
	chkcache(color, oldcolor);
	color = conv_color(color);
	fprintf(tfp, "textcolr %d;\n", color);
}

static void
textsize(double size)
{
	static double oldsize = UNDEFVALUE;
	chkcache(size, oldsize);
	/* adjust for any differences in ppi (Fig 2.x vs 3.x) */
	fprintf(tfp, "charheight %d;\n",
			round( 10 * size * ppi / 1200.0 / fontmag));
}

static void
textangle(double angle)
{
	int c, s;
	static double oldangle = UNDEFVALUE;
	chkcache(angle, oldangle);
	c = round(1200*cos(angle)); s = round(1200*sin(angle));
	fprintf(tfp, "charori (%d,%d) (%d,%d);\n", -s, c, c, s);
}

static void
cgm_text(int x, int y, char *text)
{
	fprintf(tfp, "text ");
	_pos(x, y);
	fprintf(tfp, " final '");
	/* if text contains a "'", must escape it(them) */
	while ( *text ) {
		fputc(*text,tfp);
		if ( *text == '\'' )
			fputc('\'',tfp);
		text++;
	}
	fprintf(tfp,"';\n");
}

void
gencgm_text(F_text *t)
{
	/* print any comments prefixed with "%" */
	print_comments("% ",t->comments, " %");

	fprintf(tfp,"%% Text %%\n");
	textfont(t->font, t->flags);
	texttype(t->type);
	textcolr(t->color);
	textsize(t->size);
	textangle(t->angle);
	cgm_text(t->base_x, t->base_y, t->cstring);
}

struct driver dev_cgm = {
	gencgm_option,
	gencgm_start,
	(void(*)(float,float))gendev_null,
	gencgm_arc,
	gencgm_ellipse,
	gencgm_line,
	gencgm_spline,
	gencgm_text,
	gencgm_end,
	INCLUDE_TEXT
};
