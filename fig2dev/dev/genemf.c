/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1999 by Philippe Bekaert
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
 * genemf.c -- convert fig to Enhanced MetaFile
 *
 * Revision History:
 *
 *   2001/10/03 - Added htof functions for big endian machines (M. Schrick)
 *
 *   2001/09/24 - Added filled polygons, circles and elipses (M. Schrick)
 *
 *   2001/03/04 - Created from gencgm.c (M. Schrick)
 *
 * Limitations:
 *
 * - old style splines are not supported by this driver. New style
 *   (X) splines are automatically converted to polylines and thus
 *   are supported.
 *
 * - EMFv1 doesn't support bitmap images, so forget your picbox polylines.
 *
 * - EMFv1 doesn't support the dash-triple-dotted linestyle. Such lines
 *   will appear as solid lines.
 *
 * - EMFv1 supports only 6 patterns which are different from FIG's 22
 *   fill pattern.
 *
 * - EMFv1 doesn't support line cap and join styles. The correct
 *   appearance of arrows depends on the cap style the viewer uses (not
 *   known at conversion time, but can be set using the -r driver option).
 *
 * - a EMF file may look quite different when viewed with different
 *   EMF capable drawing programs. This is especially so for text:
 *   the text font e.g. needs to be recognized and supported by the
 *   viewer.  Same is True for special characters in text strings, text
 *   orientation, ...  This driver also assumes that the background
 *   remains visible behind hatched polygons for correct appearance of
 *   pattern filled shapes with non-white background. This is not always
 *   True ...
 *
 * Known bugs:
 *
 * - parts of objects close to arrows on wide curves might sometimes be
 *   hidden.  (They disappear together with part of the polyline covered
 *   by the arrow that has to be erased in order to get a clean arrow
 *   tip). This problem requires a different arrow drawing strategy.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "fig2dev.h"
#include "object.h"
#include "genemf.h"

#define UNDEFVALUE	-100	/* UNDEFined attribute value */
#define FILL_COLOR_INDEX 999 	/* special color index for solid filled shapes.
				 * see fillshade() and conv_color(). */
#define EPSILON		1e-4	/* small floating point value */

typedef enum {EMH_RECORD, EMH_DATA} emh_flag;
static ENHMETAHEADER emh;	/* The Enhanced MetaFile header, which is the
				 * first record in the file must be updated
				 * to reflect the total number of records and
				 * the total number of bytes.  This record
				 * conatins little endian data and should not
				 * be used internally without a conversion. */
/* Internal variables in host format */
static ulong  emh_nBytes;	/* Size of the metafile in bytes */
static ulong  emh_nRecords;	/* Number of records in the metafile */
static ushort emh_nHandles;	/* Number of handles in the handle table */


static int oldpenhandle;	/* The last pen handle used */

static int rounded_arrows;	/* If rounded_arrows is False, the position
				 * of arrows will be corrected for
				 * compensating line width effects. This
				 * correction is not needed if arrows appear
				 * rounded with the used EMF viewer.
				 * See -r driver command line option. */

typedef struct Dir {double x, y;} Dir;
#define ARROW_INDENT_DIST	0.8
#define ARROW_POINT_DIST	1.2
/* Arrows appear this much longer with projected line caps. */
#define ARROW_EXTRA_LEN(a)	((double)a->ht / (double)a->wid * a->thickness)

/* EMF patterns are numbered 1-6 (I use 0 for nonexistant patterns) */
int emf_map_pattern [22] = { 0, 0, 0, 4,
			 3, 6, 0, 0,
			 1, 2, 5, 0,
			 0, 0, 0, 0,
			 0, 0, 0, 0,
			 0, 0 };

/* sets EMF interior style */
typedef enum {SOLID, HOLLOW, HATCH, EMPTY, UNDEF} INTSTYLE;


/*~~~~~|><|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

static int ltFonts[] = {	/* Convert from LaTeX fonts to Post Script */
		0,		/* default */
		0,		/* Roman */
		2,		/* Bold */
		1,		/* Italic */
		4,		/* Modern */
		12,		/* Typewriter */
		};

static char *lfFaceName[] = {
		"Times",		/* Times-Roman */
		"Times",		/* Times-Italic */
		"Times",		/* Times-Bold */
		"Times",		/* Times-BoldItalic */
		"AvantGarde",		/* AvantGarde */
		"AvantGarde",		/* AvantGarde-BookOblique */
		"AvantGarde",		/* AvantGarde-Demi */
		"AvantGarde",		/* AvantGarde-DemiOblique */
		"Bookman",		/* Bookman-Light */
		"Bookman",		/* Bookman-LightItalic */
		"Bookman",		/* Bookman-Demi */
		"Bookman",		/* Bookman-DemiItalic */
		"Courier",		/* Courier */
		"Courier",		/* Courier-Oblique */
		"Courier",		/* Courier-Bold */
		"Courier",		/* Courier-BoldItalic */
		"Helvetica",		/* Helvetica */
		"Helvetica",		/* Helvetica-Oblique */
		"Helvetica",		/* Helvetica-Bold */
		"Helvetica",		/* Helvetica-BoldOblique */
		"Helvetica-Narrow",	/* Helvetica-Narrow */
		"Helvetica-Narrow",	/* Helvetica-Narrow-Oblique */
		"Helvetica-Narrow",	/* Helvetica-Narrow-Bold */
		"Helvetica-Narrow",	/* Helvetica-Narrow-BoldOblique */
		"NewCenturySchlbk",	/* NewCenturySchlbk-Roman */
		"NewCenturySchlbk",	/* NewCenturySchlbk-Italic */
		"NewCenturySchlbk",	/* NewCenturySchlbk-Bold */
		"NewCenturySchlbk",	/* NewCenturySchlbk-BoldItalic */
		"Palatino",		/* Palatino-Roman */
		"Palatino",		/* Palatino-Italic */
		"Palatino",		/* Palatino-Bold */
		"Palatino",		/* Palatino-BoldItalic */
		"Symbol",		/* Symbol */
		"ZapfChancery",		/* ZapfChancery-MediumItalic */
		"ZapfDingbats"		/* ZapfDingbats */
		};

static int	lfWeight[] = {
		FW_NORMAL,		/* Times-Roman */
		FW_NORMAL,		/* Times-Italic */
		FW_BOLD,		/* Times-Bold */
		FW_BOLD,		/* Times-BoldItalic */
		FW_NORMAL,		/* AvantGarde */
		FW_NORMAL,		/* AvantGarde-BookOblique */
		FW_DEMIBOLD,		/* AvantGarde-Demi */
		FW_DEMIBOLD,		/* AvantGarde-DemiOblique */
		FW_LIGHT,		/* Bookman-Light */
		FW_LIGHT,		/* Bookman-LightItalic */
		FW_DEMIBOLD,		/* Bookman-Demi */
		FW_DEMIBOLD,		/* Bookman-DemiItalic */
		FW_NORMAL,		/* Courier */
		FW_NORMAL,		/* Courier-Oblique */
		FW_BOLD,		/* Courier-Bold */
		FW_BOLD,		/* Courier-BoldItalic */
		FW_NORMAL,		/* Helvetica */
		FW_NORMAL,		/* Helvetica-Oblique */
		FW_BOLD,		/* Helvetica-Bold */
		FW_BOLD,		/* Helvetica-BoldOblique */
		FW_NORMAL,		/* Helvetica-Narrow */
		FW_NORMAL,		/* Helvetica-Narrow-Oblique */
		FW_BOLD,		/* Helvetica-Narrow-Bold */
		FW_BOLD,		/* Helvetica-Narrow-BoldOblique */
		FW_NORMAL,		/* NewCenturySchlbk-Roman */
		FW_NORMAL,		/* NewCenturySchlbk-Italic */
		FW_BOLD,		/* NewCenturySchlbk-Bold */
		FW_BOLD,		/* NewCenturySchlbk-BoldItalic */
		FW_NORMAL,		/* Palatino-Roman */
		FW_NORMAL,		/* Palatino-Italic */
		FW_BOLD,		/* Palatino-Bold */
		FW_BOLD,		/* Palatino-BoldItalic */
		FW_NORMAL,		/* Symbol */
		FW_NORMAL,		/* ZapfChancery-MediumItalic */
		FW_NORMAL		/* ZapfDingbats */
		};

static uchar	lfItalic[] = {
		False,			/* Times-Roman */
		True,			/* Times-Italic */
		False,			/* Times-Bold */
		True,			/* Times-BoldItalic */
		False,			/* AvantGarde */
		True,			/* AvantGarde-BookOblique */
		False,			/* AvantGarde-Demi */
		True,			/* AvantGarde-DemiOblique */
		False,			/* Bookman-Light */
		True,			/* Bookman-LightItalic */
		False,			/* Bookman-Demi */
		True,			/* Bookman-DemiItalic */
		False,			/* Courier */
		True,			/* Courier-Oblique */
		False,			/* Courier-Bold */
		True,			/* Courier-BoldItalic */
		False,			/* Helvetica */
		True,			/* Helvetica-Oblique */
		False,			/* Helvetica-Bold */
		True,			/* Helvetica-BoldOblique */
		False,			/* Helvetica-Narrow */
		True,			/* Helvetica-Narrow-Oblique */
		False,			/* Helvetica-Narrow-Bold */
		True,			/* Helvetica-Narrow-BoldOblique */
		False,			/* NewCenturySchlbk-Roman */
		True,			/* NewCenturySchlbk-Italic */
		False,			/* NewCenturySchlbk-Bold */
		True,			/* NewCenturySchlbk-BoldItalic */
		False,			/* Palatino-Roman */
		True,			/* Palatino-Italic */
		False,			/* Palatino-Bold */
		True,			/* Palatino-BoldItalic */
		False,			/* Symbol */
		False,			/* ZapfChancery-MediumItalic */
		False			/* ZapfDingbats */
		};

static uchar	lfCharSet[] = {
		ANSI_CHARSET,		/* Times-Roman */
		ANSI_CHARSET,		/* Times-Italic */
		ANSI_CHARSET,		/* Times-Bold */
		ANSI_CHARSET,		/* Times-BoldItalic */
		ANSI_CHARSET,		/* AvantGarde */
		ANSI_CHARSET,		/* AvantGarde-BookOblique */
		ANSI_CHARSET,		/* AvantGarde-Demi */
		ANSI_CHARSET,		/* AvantGarde-DemiOblique */
		ANSI_CHARSET,		/* Bookman-Light */
		ANSI_CHARSET,		/* Bookman-LightItalic */
		ANSI_CHARSET,		/* Bookman-Demi */
		ANSI_CHARSET,		/* Bookman-DemiItalic */
		ANSI_CHARSET,		/* Courier */
		ANSI_CHARSET,		/* Courier-Oblique */
		ANSI_CHARSET,		/* Courier-Bold */
		ANSI_CHARSET,		/* Courier-BoldItalic */
		ANSI_CHARSET,		/* Helvetica */
		ANSI_CHARSET,		/* Helvetica-Oblique */
		ANSI_CHARSET,		/* Helvetica-Bold */
		ANSI_CHARSET,		/* Helvetica-BoldOblique */
		ANSI_CHARSET,		/* Helvetica-Narrow */
		ANSI_CHARSET,		/* Helvetica-Narrow-Oblique */
		ANSI_CHARSET,		/* Helvetica-Narrow-Bold */
		ANSI_CHARSET,		/* Helvetica-Narrow-BoldOblique */
		ANSI_CHARSET,		/* NewCenturySchlbk-Roman */
		ANSI_CHARSET,		/* NewCenturySchlbk-Italic */
		ANSI_CHARSET,		/* NewCenturySchlbk-Bold */
		ANSI_CHARSET,		/* NewCenturySchlbk-BoldItalic */
		ANSI_CHARSET,		/* Palatino-Roman */
		ANSI_CHARSET,		/* Palatino-Italic */
		ANSI_CHARSET,		/* Palatino-Bold */
		ANSI_CHARSET,		/* Palatino-BoldItalic */
		SYMBOL_CHARSET,		/* Symbol */
		SYMBOL_CHARSET,		/* ZapfChancery-MediumItalic */
		SYMBOL_CHARSET		/* ZapfDingbats */
		};

static uchar	lfPitchAndFamily[] = {
	VARIABLE_PITCH | FF_ROMAN,	/* Times-Roman */
	VARIABLE_PITCH | FF_ROMAN,	/* Times-Italic */
	VARIABLE_PITCH | FF_ROMAN,	/* Times-Bold */
	VARIABLE_PITCH | FF_ROMAN,	/* Times-BoldItalic */
	VARIABLE_PITCH | FF_ROMAN,	/* AvantGarde */
	VARIABLE_PITCH | FF_ROMAN,	/* AvantGarde-BookOblique */
	VARIABLE_PITCH | FF_ROMAN,	/* AvantGarde-Demi */
	VARIABLE_PITCH | FF_ROMAN,	/* AvantGarde-DemiOblique */
	VARIABLE_PITCH | FF_ROMAN,	/* Bookman-Light */
	VARIABLE_PITCH | FF_ROMAN,	/* Bookman-LightItalic */
	VARIABLE_PITCH | FF_ROMAN,	/* Bookman-Demi */
	VARIABLE_PITCH | FF_ROMAN,	/* Bookman-DemiItalic */
	   FIXED_PITCH | FF_MODERN,	/* Courier */
	   FIXED_PITCH | FF_MODERN,	/* Courier-Oblique */
	   FIXED_PITCH | FF_MODERN,	/* Courier-Bold */
	   FIXED_PITCH | FF_MODERN,	/* Courier-BoldItalic */
	VARIABLE_PITCH | FF_ROMAN,	/* Helvetica */
	VARIABLE_PITCH | FF_ROMAN,	/* Helvetica-Oblique */
	VARIABLE_PITCH | FF_ROMAN,	/* Helvetica-Bold */
	VARIABLE_PITCH | FF_ROMAN,	/* Helvetica-BoldOblique */
	VARIABLE_PITCH | FF_ROMAN,	/* Helvetica-Narrow */
	VARIABLE_PITCH | FF_ROMAN,	/* Helvetica-Narrow-Oblique */
	VARIABLE_PITCH | FF_ROMAN,	/* Helvetica-Narrow-Bold */
	VARIABLE_PITCH | FF_ROMAN,	/* Helvetica-Narrow-BoldOblique */
	VARIABLE_PITCH | FF_ROMAN,	/* NewCenturySchlbk-Roman */
	VARIABLE_PITCH | FF_ROMAN,	/* NewCenturySchlbk-Italic */
	VARIABLE_PITCH | FF_ROMAN,	/* NewCenturySchlbk-Bold */
	VARIABLE_PITCH | FF_ROMAN,	/* NewCenturySchlbk-BoldItalic */
	VARIABLE_PITCH | FF_ROMAN,	/* Palatino-Roman */
	VARIABLE_PITCH | FF_ROMAN,	/* Palatino-Italic */
	VARIABLE_PITCH | FF_ROMAN,	/* Palatino-Bold */
	VARIABLE_PITCH | FF_ROMAN,	/* Palatino-BoldItalic */
	VARIABLE_PITCH | FF_ROMAN,	/* Symbol */
	VARIABLE_PITCH | FF_ROMAN,	/* ZapfChancery-MediumItalic */
	VARIABLE_PITCH | FF_ROMAN,	/* ZapfDingbats */
		};


/*~~~~~|><|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


static void _arcctr();
static void _circle();
static void _ellipse();
static void _fillcolr();
static void _line();
static void _pos();
static void _relpos();
static void arcboxinterior();
static char *arctype();
static void arc_arrow();
static void arc_arrow_adjust();
static void arc_midpoint();
static double arc_radius();
static void arc_rotate();
static void arcwitharrows();
static void arcoutline();
static void arcinterior();
static void arc_reverse();
static void arcboxsetup();
static void arcboxoutline();
static void arrow();
static double arrow_length();
static void circle();
static int conv_color();
static int conv_fontindex();
static int conv_linetype();
static int conv_pattern_index();
static int cwarc();
static int direction();
static double distance();
static void edgeattr();
static void edgecolr();
static void edgetype();
static void edgevis();
static void edgewidth();
static void ellipse();
static void ellipsetup();
static size_t emh_write();
static void fillcolr();
static void fillcolrgb();
static void fillshade();
static void getrgb();
static void hatchindex();
static int icprod();
static void intstyle();
static void line();
static void lineattr();
static void picbox();
static void point();
static void polygon();
static void polyline();
static int  polyline_arrow_adjust();
static void pos();
static void pos2point();
static void rect();
static void rotate();
static void shape();
static void shape_interior();
static void text();
static void textangle();
static void textcolr();
static void textfont();
static void texttype();
static void textunicode();
static void translate();



static void _arcctr(cx, cy, x1, y1, x2, y2, r)
    int cx, cy, x1, y1, x2, y2, r;
{
  /*fprintf(tfp, "arcctr ");*/
  _pos(cx, cy);
  /*fprintf(tfp, " ");*/
  _relpos(x2, y2);
  /*fprintf(tfp, " ");*/
  _relpos(x1, y1);
  /*fprintf(tfp, " %d;\n", r);*/
}


static void _circle(cx, cy, r)
    int cx, cy, r;
{
  EMRELLIPSE em_el;

  em_el.emr.iType = htofl(EMR_ELLIPSE);
  em_el.emr.nSize = htofl(sizeof(EMRELLIPSE));
  em_el.rclBox.left   = htofl(cx + r);
  em_el.rclBox.top    = htofl(cy + r);
  em_el.rclBox.right  = htofl(cx - r);
  em_el.rclBox.bottom = htofl(cy - r);

# ifdef __EMF_DEBUG__
    fprintf(stderr, "Circle rclBox (ltrb): %d %d %d %d\n", 
	ftohl(em_el.rclBox.left),  ftohl(em_el.rclBox.top),
	ftohl(em_el.rclBox.right), ftohl(em_el.rclBox.bottom));
# endif

  emh_write(&em_el, sizeof(EMRRECTANGLE), 1, EMH_RECORD);
}


/* This procedure does not draw rotated ellipses yet.  It appears that*/
/* they will have to be rendered for EMF files.*/
static void _ellipse(cx, cy, x1, y1, x2, y2)
    int cx, cy, x1, y1, x2, y2;
{
  EMRELLIPSE em_el;

  em_el.emr.iType = htofl(EMR_ELLIPSE);
  em_el.emr.nSize = htofl(sizeof(EMRELLIPSE));
  em_el.rclBox.left   = htofl(x1);
  em_el.rclBox.top    = htofl(y1 - (y2-y1));
  em_el.rclBox.right  = htofl(x2 + (x2-x1));
  em_el.rclBox.bottom = htofl(y2);

# ifdef __EMF_DEBUG__
    fprintf(stderr, "Ellipse rclBox (ltrb): %d %d %d %d\n", 
	ftohl(em_el.rclBox.left),  ftohl(em_el.rclBox.top),
	ftohl(em_el.rclBox.right), ftohl(em_el.rclBox.bottom));
# endif

  emh_write(&em_el, sizeof(EMRRECTANGLE), 1, EMH_RECORD);
}


static int oldfillcolor = UNDEFVALUE;
/* updates unconditionally */
static void _fillcolr(color)
    int color;
{
  oldfillcolor = color;
  color = conv_color(color);
  /*fprintf(tfp, "fillcolr %d;\n", color);*/
}


static void _line(x1, y1, x2, y2)
    int x1, y1, x2, y2;
{
  /*fprintf(tfp, "line "); _pos(x1, y1); fprintf(tfp, " "); _pos(x2, y2); fprintf(tfp, ";\n");*/
}

/* coordinates are translated such that the lower left corner has
 * coordinates (0,0). we use fig units for spacing (no coordinate scaling)
 * that means: 1200 units/inch. */
static void _pos(x, y)
   int	x;
   int	y;
{
    /*fprintf(tfp, "(%d,%d)", x-llx, ury-y);*/
}


/* Only reverses y if y axis points down (relative position) */
static void _relpos(x, y)
   int	x;
   int	y;
{
    /*fprintf(tfp, "(%d,%d)", x, -y);*/
}


static void arcboxinterior(l)
    F_line *l;
{
  int llx, lly, urx, ury, r;
  arcboxsetup(l, &llx, &lly, &urx, &ury, &r);

  /*fprintf(tfp, "polygon ");*/
  _pos(llx  , lly+r); /*fprintf(tfp, " ");*/
  _pos(llx  , ury-r); /*fprintf(tfp, " ");*/
  _pos(llx+r, ury-r); /*fprintf(tfp, " ");*/
  _pos(llx+r, ury  ); /*fprintf(tfp, "\n    ");*/
  _pos(urx-r, ury  ); /*fprintf(tfp, " ");*/
  _pos(urx-r, ury-r); /*fprintf(tfp, " ");*/
  _pos(urx  , ury-r); /*fprintf(tfp, " ");*/
  _pos(urx  , lly+r); /*fprintf(tfp, "\n    ");*/
  _pos(urx-r, lly+r); /*fprintf(tfp, " ");*/
  _pos(urx-r, lly  ); /*fprintf(tfp, " ");*/
  _pos(llx+r, lly  ); /*fprintf(tfp, " ");*/
  _pos(llx+r, lly+r); /*fprintf(tfp, ";\n");*/

  _circle(llx+r, ury-r, r);
  _circle(urx-r, ury-r, r);
  _circle(urx-r, lly+r, r);
  _circle(llx+r, lly+r, r);
}


static char *arctype(type)
    int type;
{
  switch (type) {
  case T_OPEN_ARC:
    return "chord";
  case T_PIE_WEDGE_ARC:
    return "pie";
  default:
    fprintf(stderr, "Unsupported fig arc type %d.\n", type);
  }
  return "unknown";
}


/* Draws arc arrow ending at p and starting at q. */
static void arc_arrow(p, q, arw, arc)
    F_pos	*p;
    F_point	*q;
    F_arrow	*arw;
    F_arc	*arc;
{
  F_point P;
  Dir dir; double d;

  if (!arw) return;

  pos2point(&P, p);
  direction(&P, q, &dir, &d);
  arrow(&P, arw, (F_line *)arc, &dir);
  if (arw->type == 0) {
    /* Draw middle leg of old-style stick arrow */
    double f = arrow_length(arw);
    P.x = round(P.x - f * dir.x);
    P.y = round(P.y - f * dir.y);
    line(q, &P);
  }
}


/* Replaces p by the starting point of the arc arrow ending at p. */
static void arc_arrow_adjust(p, cx, cy, r, arw, dir)
    F_point *p;
    double cx, cy, r;
    F_arrow *arw;
    double dir;
{
  double l;
  if (!arw) return;

  l = arw->type != 0 ? arrow_length(arw) : arw->ht;
  arc_rotate(p, cx, cy, r, dir * M_PI * l / (2. * r + l));
}

/* Computes midpoint of p1 and p2 on arc. */
static void arc_midpoint(mid, p1, p2, cx, cy, r)
    F_point *mid;
    F_point *p1;
    F_point *p2;
    double cx, cy, r;
{
  Dir dir; double d;
  direction(p1, p2, &dir, &d);
  *mid = *p1;
  arc_rotate(mid, cx, cy, r, M_PI * d / (2. * r + d) / 2.);
}


static double arc_radius(a)
    F_arc *a;
{
  return (distance((double)a->point[0].x, (double)a->point[0].y, 
  			a->center.x, a->center.y) +
	  distance((double)a->point[1].x, (double)a->point[1].y,
	  		a->center.x, a->center.y) +
	  distance((double)a->point[2].x, (double)a->point[2].y,
	  		a->center.x, a->center.y)) / 3.;
}


/* Rotates the point p counter clockwise along the arc with center c and 
 * radius r. */
static void arc_rotate(p, cx, cy, r, angle)
    F_point *p;
    double cx, cy, r, angle;
{
  double x = p->x, y = p->y;
  translate(&x, &y, -cx, -cy);
  rotate(&x, &y, angle);
  translate(&x, &y, +cx, +cy);
  p->x = round(x); p->y = round(y);
}


static void arcwitharrows(a)
    F_arc *a;
{
  F_point p0, p1, p2;
  double r = arc_radius(a);

  pos2point(&p0, &a->point[0]);
  arc_arrow_adjust(&p0, a->center.x, a->center.y, r, a->back_arrow, +1.);

  pos2point(&p2, &a->point[2]);
  arc_arrow_adjust(&p2, a->center.x, a->center.y, r, a->for_arrow, -1.);

  /* make sure p1 lays between p0 and p2 */
  arc_midpoint(&p1, &p0, &p2, a->center.x, a->center.y, r);

  /*fprintf(tfp, "arc3pt ");*/
  point(&p0); /*fprintf(tfp, " ");*/
  point(&p1); /*fprintf(tfp, " ");*/
  point(&p2); /*fprintf(tfp, ";\n");*/

  arc_arrow(&a->point[0], &p0, a->back_arrow, a);
  arc_arrow(&a->point[2], &p2, a->for_arrow, a);
}


static void arcoutline(a)
    F_arc *a;
{
  if ((a->type == T_OPEN_ARC) && (a->thickness != 0) 
  	&& (a->back_arrow || a->for_arrow)) {
    arcwitharrows(a);
    return;
  }

  /*fprintf(tfp, "arc3pt ");*/
  pos(&a->point[0]); /*fprintf(tfp, " ");*/
  pos(&a->point[1]); /*fprintf(tfp, " ");*/
  pos(&a->point[2]); /*fprintf(tfp, ";\n");*/

  switch (a->type) {
  case T_PIE_WEDGE_ARC:
    /*fprintf(tfp, "line ");	(* close the pie wedge */
    pos(&a->point[2]); /*fprintf(tfp, " ");*/
    _pos(round(a->center.x), round(a->center.y)); /*fprintf(tfp, " ");*/
    pos(&a->point[0]); /*fprintf(tfp, ";\n");*/
    break;
  case T_OPEN_ARC:
  default:
    /* don't draw anything more than the arc itself. */
    break;
  }
}


static void arcinterior(a)
    F_arc *a;
{
  /*fprintf(tfp, "arc3ptclose ");*/
  pos(&a->point[0]); /*fprintf(tfp, " ");*/
  pos(&a->point[1]); /*fprintf(tfp, " ");*/
  pos(&a->point[2]); /*fprintf(tfp, " %s;\n", arctype(a->type));*/
}


/* Reverses arc direction by swapping endpoints and arrows */
static void arc_reverse(arc)
    F_arc *arc;
{
  F_arrow *arw;
  F_pos pp;
  pp = arc->point[0]; arc->point[0] = arc->point[2]; arc->point[2] = pp;
  arw = arc->for_arrow; arc->for_arrow = arc->back_arrow; arc->back_arrow = arw;
}


static void arcboxsetup(l, llx, lly, urx, ury, r)
    F_line *l;
    int *llx, *lly, *urx, *ury, *r;
{
  *llx = l->points->x;
  *lly = l->points->y;
  *urx = l->points->next->next->x;
  *ury = l->points->next->next->y;
  *r = l->radius;
  if (*llx > *urx) {int t = *urx; *urx = *llx; *llx = t;}
  if (*lly > *ury) {int t = *ury; *ury = *lly; *lly = t;}
}


static void arcboxoutline(l)
    F_line *l;
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


/* Draws an arrow ending at point p pointing into direction dir.
 * type and attributes as required by a and l.  This routine works
 * for both lines and arc (in that case l should be a F_arc *). */
static void arrow(P, a, l, dir)
    F_point	*P;
    F_arrow	*a;
    F_line	*l;
    Dir		*dir;
{
  F_point s1, s2, t, p;
  EMRPOLYLINE em_pl;		/* Little endian format */
  POINTL      aptl[4];		/* Maximum of four points required for arrows */

  RECTL       rclBounds;	/* Bounding box in host format */
  ulong       cptl;		/* Number of points in the array */

  p = *P;
  if (!rounded_arrows) {
    /* Move the arrow backwards in order to let it end at the correct spot */
    double f = ARROW_EXTRA_LEN(a);
    p.x = round(p.x - f * dir->x);
    p.y = round(p.y - f * dir->y);
  }

  if (a->type == 0) {		/* Old style stick arrow */
    lineattr(0, (int)a->thickness, l->pen_color, 0, 0);
  } else {			/* Polygonal arrows */
    intstyle(SOLID);
    switch (a->style) {
    case 0:
      fillcolr(7);
      break;
    case 1:
      fillcolr(l->pen_color);
      break;
    default:
      fprintf(stderr, "Unsupported fig arrow style %d !!\n", a->style);
    }
    edgeattr(1, 0, (int)a->thickness, l->pen_color);
  }

  /* Start the bounding box */

  aptl[1].x = htofl(p.x);
  rclBounds.left  = P->x;		/* Use original point for bounding */
  rclBounds.right = P->x;
  aptl[1].y = htofl(p.y);
  rclBounds.top    = P->y;
  rclBounds.bottom = P->y;

  switch (a->type) {

  case 0:				/* Stick type */
    em_pl.emr.iType = htofl(EMR_POLYLINE);
    cptl = 3;

    s1.x = round(p.x - a->ht * dir->x + a->wid*0.5 * dir->y);
    s1.y = round(p.y - a->ht * dir->y - a->wid*0.5 * dir->x);
    s2.x = round(p.x - a->ht * dir->x - a->wid*0.5 * dir->y);
    s2.y = round(p.y - a->ht * dir->y + a->wid*0.5 * dir->x);

    aptl[0].x = htofl(s1.x);
    if (rclBounds.left > s1.x) {
      rclBounds.left  = s1.x;
    } else if (rclBounds.right < s1.x) {
      rclBounds.right = s1.x;
    }
    aptl[0].y = htofl(s1.y);
    if (rclBounds.top > s1.y) {
      rclBounds.top    = s1.y;
    } else if (rclBounds.bottom < s1.y) {
      rclBounds.bottom = s1.y;
    }

    aptl[2].x = htofl(s2.x);
    if (rclBounds.left > s2.x) {
      rclBounds.left  = s2.x;
    } else if (rclBounds.right < s2.x) {
      rclBounds.right = s2.x;
    }
    aptl[2].y = htofl(s2.y);
    if (rclBounds.top > s2.y) {
      rclBounds.top    = s2.y;
    } else if (rclBounds.bottom < s2.y) {
      rclBounds.bottom = s2.y;
    }
    break;

  case 1:				/* Closed triangle */
    em_pl.emr.iType = htofl(EMR_POLYGON);
    cptl = 3;

    s1.x = round(p.x - a->ht * dir->x + a->wid*0.5 * dir->y);
    s1.y = round(p.y - a->ht * dir->y - a->wid*0.5 * dir->x);
    s2.x = round(p.x - a->ht * dir->x - a->wid*0.5 * dir->y);
    s2.y = round(p.y - a->ht * dir->y + a->wid*0.5 * dir->x);

    aptl[0].x = htofl(s1.x);
    if (rclBounds.left > s1.x) {
      rclBounds.left  = s1.x;
    } else if (rclBounds.right < s1.x) {
      rclBounds.right = s1.x;
    }
    aptl[0].y = htofl(s1.y);
    if (rclBounds.top > s1.y) {
      rclBounds.top    = s1.y;
    } else if (rclBounds.bottom < s1.y) {
      rclBounds.bottom = s1.y;
    }

    aptl[2].x = htofl(s2.x);
    if (rclBounds.left > s2.x) {
      rclBounds.left  = s2.x;
    } else if (rclBounds.right < s2.x) {
      rclBounds.right = s2.x;
    }
    aptl[2].y = htofl(s2.y);
    if (rclBounds.top > s2.y) {
      rclBounds.top    = s2.y;
    } else if (rclBounds.bottom < s2.y) {
      rclBounds.bottom = s2.y;
    }
    break;

  case 2:				/* Indented hat */
    em_pl.emr.iType = htofl(EMR_POLYGON);
    cptl = 4;

    s1.x = round(p.x - a->ht*ARROW_POINT_DIST * dir->x
    		     + a->wid*0.5 * dir->y);
    s1.y = round(p.y - a->ht*ARROW_POINT_DIST * dir->y
		     - a->wid*0.5 * dir->x);
    s2.x = round(p.x - a->ht*ARROW_POINT_DIST * dir->x
		     - a->wid*0.5 * dir->y);
    s2.y = round(p.y - a->ht*ARROW_POINT_DIST * dir->y
		     + a->wid*0.5 * dir->x);

    t.x = round(p.x - a->ht * dir->x);
    t.y = round(p.y - a->ht * dir->y);

    aptl[3].x = htofl(t.x);
    if (rclBounds.left > t.x) {
      rclBounds.left  = t.x;
    } else if (rclBounds.right < t.x) {
      rclBounds.right = t.x;
    }
    aptl[3].y = htofl(t.y);
    if (rclBounds.top > t.y) {
      rclBounds.top    = t.y;
    } else if (rclBounds.bottom < t.y) {
      rclBounds.bottom = t.y;
    }

    aptl[0].x = htofl(s1.x);
    if (rclBounds.left > s1.x) {
      rclBounds.left  = s1.x;
    } else if (rclBounds.right < s1.x) {
      rclBounds.right = s1.x;
    }
    aptl[0].y = htofl(s1.y);
    if (rclBounds.top > s1.y) {
      rclBounds.top    = s1.y;
    } else if (rclBounds.bottom < s1.y) {
      rclBounds.bottom = s1.y;
    }

    aptl[2].x = htofl(s2.x);
    if (rclBounds.left > s2.x) {
      rclBounds.left  = s2.x;
    } else if (rclBounds.right < s2.x) {
      rclBounds.right = s2.x;
    }
    aptl[2].y = htofl(s2.y);
    if (rclBounds.top > s2.y) {
      rclBounds.top    = s2.y;
    } else if (rclBounds.bottom < s2.y) {
      rclBounds.bottom = s2.y;
    }
    break;

  case 3:				/* Pointed hat */
    em_pl.emr.iType = htofl(EMR_POLYGON);
    cptl = 4;
    
    s1.x = round(p.x - a->ht*ARROW_INDENT_DIST * dir->x
    		     + a->wid*0.5 * dir->y);
    s1.y = round(p.y - a->ht*ARROW_INDENT_DIST * dir->y
		     - a->wid*0.5 * dir->x);
    s2.x = round(p.x - a->ht*ARROW_INDENT_DIST * dir->x
		     - a->wid*0.5 * dir->y);
    s2.y = round(p.y - a->ht*ARROW_INDENT_DIST * dir->y
		     + a->wid*0.5 * dir->x);

    t.x = round(p.x - a->ht * dir->x);
    t.y = round(p.y - a->ht * dir->y);

    aptl[3].x = htofl(t.x);
    if (rclBounds.left > t.x) {
      rclBounds.left  = t.x;
    } else if (rclBounds.right < t.x) {
      rclBounds.right = t.x;
    }
    aptl[3].y = htofl(t.y);
    if (rclBounds.top > t.y) {
      rclBounds.top    = t.y;
    } else if (rclBounds.bottom < t.y) {
      rclBounds.bottom = t.y;
    }

    aptl[0].x = htofl(s1.x);
    if (rclBounds.left > s1.x) {
      rclBounds.left  = s1.x;
    } else if (rclBounds.right < s1.x) {
      rclBounds.right = s1.x;
    }
    aptl[0].y = htofl(s1.y);
    if (rclBounds.top > s1.y) {
      rclBounds.top    = s1.y;
    } else if (rclBounds.bottom < s1.y) {
      rclBounds.bottom = s1.y;
    }

    aptl[2].x = htofl(s2.x);
    if (rclBounds.left > s2.x) {
      rclBounds.left  = s2.x;
    } else if (rclBounds.right < s2.x) {
      rclBounds.right = s2.x;
    }
    aptl[2].y = htofl(s2.y);
    if (rclBounds.top > s2.y) {
      rclBounds.top    = s2.y;
    } else if (rclBounds.bottom < s2.y) {
      rclBounds.bottom = s2.y;
    }
    break;

  default:
    fprintf(stderr, "Unsupported fig arrow type %d.\n", a->type);

  }/* end case */

# ifdef __EMF_DEBUG__
    fprintf(stderr, "Arrowhead %d rclBounds (ltrb): %d %d %d %d\n", 
      cptl,
      rclBounds.left,  rclBounds.top,
      rclBounds.right, rclBounds.bottom);
# endif

  em_pl.rclBounds.left   = htofl(rclBounds.left);	/* Store bounding box */
  em_pl.rclBounds.right  = htofl(rclBounds.right);
  em_pl.rclBounds.top    = htofl(rclBounds.top);
  em_pl.rclBounds.bottom = htofl(rclBounds.bottom);
  em_pl.cptl = htofl(cptl);				/* Number of points */

  em_pl.emr.nSize = htofl(sizeof(EMRPOLYLINE) + cptl*sizeof(POINTL));
  emh_write(&em_pl, sizeof(EMRPOLYLINE), 1, EMH_RECORD);
  emh_write(aptl, sizeof(POINTL), cptl, EMH_DATA);
}


/* Returns length of the arrow. used to shorten lines/arcs at
 * an end where an arrow needs to be drawn. */
static double arrow_length(a)
    F_arrow *a;
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


/* Piece of code to avoid unnecessary attribute changes */
#define chkcache(val, cachedval) 	\
  if (val == cachedval)			\
    return;				\
  else					\
    cachedval = val;

static void circle(e)
    F_ellipse *e;
{
  _circle(e->center.x, e->center.y, e->radiuses.x);
}


/* Given an index into either the standard color list or into the
 * user defined color list, return the hex RGB value of the color. */
static int conv_color(colorIndex)
  int colorIndex;
{
  extern User_color user_colors[];
  int   rgb;
  static int rgbColors[NUM_STD_COLS] = {
    0x000000, 0x0000ff, 0x00ff00, 0x00ffff, 0xff0000, 0xff00ff,
    0xffff00, 0xffffff, 0x00008f, 0x0000b0, 0x0000d1, 0x87cfff,
    0x008f00, 0x00b000, 0x00d100, 0x008f8f, 0x00b0b0, 0x00d1d1,
    0x8f0000, 0xb00000, 0xd10000, 0x8f008f, 0xb000b0, 0xd100d1,
    0x803000, 0xa14000, 0xb46100, 0xff8080, 0xffa1a1, 0xffbfbf,
    0xffe0e0, 0xffd600
  };

  if (colorIndex == DEFAULT)
    rgb = rgbColors[0];
  else if (colorIndex < NUM_STD_COLS)
    rgb = rgbColors[colorIndex];
  else
    rgb = ((user_colors[colorIndex-NUM_STD_COLS].r & 0xff) << 16)
    	| ((user_colors[colorIndex-NUM_STD_COLS].g & 0xff) << 8)
    	|  (user_colors[colorIndex-NUM_STD_COLS].b & 0xff);
  return rgb;
}


/* Converts Fig font index to index into font table in the pre-amble,
 * taking into account the flags. */
static int conv_fontindex(font, flags)
    int font;
    int flags;
{
  if (flags&4) {			/* PostScript fonts */
    if (font == (-1)) {
      font = 0;				/* Default PostScript is Roman */
    } else if ((font < 0) || (font >= NUM_PS_FONTS)) {
      fprintf(stderr, "Unsupported Fig PostScript font index %d.\n", font);
      font = 0;				/* Default font */
    }
  } else {				/* LaTeX fonts */
    if ((font < 0) || (font > NUM_LATEX_FONTS)) {
      fprintf(stderr, "Unsupported Fig LaTeX font index %d.\n", font);
      font = 0;				/* Default font */
    }
    else
      font = ltFonts[font];		/* Convert to PostScript fonts */
  }
  return font;
}


/* Convert fig cap style to EMF line style. */
static int conv_capstyle(cap)
  int	cap;
{
  switch (cap) {
    case 0:	/* Butt. */
      cap = PS_ENDCAP_FLAT;
      break;
    case 1:	/* Round. */
      cap = PS_ENDCAP_ROUND;
      break;
    case 2:	/* Projecting. */
      cap = PS_ENDCAP_SQUARE;
      break;
    default:
      fprintf(stderr, "genemf: unknown cap style %d.\n", cap);
      cap = PS_ENDCAP_FLAT;
      break;
  }
  return cap;
}


/* Convert fig join style to EMF join style. */
static int conv_joinstyle(join)
  int	join;
{
  switch (join) {
    case 0:	/* Miter. */
      join = PS_JOIN_MITER;
      break;
    case 1:	/* Round. */
      join = PS_JOIN_ROUND;
      break;
    case 2:	/* Bevel. */
      join = PS_JOIN_BEVEL;
      break;
    default:
      fprintf(stderr, "genemf: unknown join style %d.\n", join);
      join = PS_JOIN_MITER;
      break;
  }
  return join;
}


/* Convert fig line style to emf line style.  EMF knows 5 styles with
 * fortunately corresond to the first 5 fig line styles.  The triple
 * dotted fig line style is reproduced as a solid line. */
static int conv_linetype(type)
  int	type;
{
  if (type < 0)
     type = -1;
  else if (type > 4)
     type %= 5;
  return type;
}


/* Converts hatch pattern index. fig knows 16 patterns, emf only 6
 * different ones. */
static int conv_pattern_index(index)
  int index;
{
  return emf_map_pattern[index];
}


/* Returns True if the arc is a clockwise arc. */
static int cwarc(a)
  F_arc *a;
{
  int x1 = a->point[1].x - a->point[0].x,
      y1 = a->point[1].y - a->point[0].y,
      x2 = a->point[2].x - a->point[1].x,
      y2 = a->point[2].y - a->point[1].y;

  return (icprod(x1,y1,x2,y2) > 0);
}


/* Computes distance and normalized direction vector from q to p.
 * returns True if the points do not coincide and False if they do. */
static int direction(p, q, dir, dist)
  F_point	*p;
  F_point	*q;
  Dir		*dir;
  double	*dist;
{
  dir->x = p->x - q->x;
  dir->y = p->y - q->y;
  *dist = sqrt((dir->x) * (dir->x) + (dir->y) * (dir->y));
  if (*dist < EPSILON)
    return False;
  dir->x /= *dist;
  dir->y /= *dist;
  return True;
}


static double distance(x1, y1, x2, y2)
  double x1, y1, x2, y2;
{
  double dx = x2-x1, dy=y2-y1;
  return sqrt(dx*dx + dy*dy);
}


/* This procedure sets the edge line attributes and selects the edge into
 * the device context. */
static void edgeattr(vis, type, width, color)
  int vis;
  int type;
  int width;
  int color;
{
  EMREXTCREATEPEN em_pn;
  EMRSELECTOBJECT em_so;

  memset(&em_pn, 0, sizeof(EMREXTCREATEPEN));
  em_pn.emr.iType = htofl(EMR_EXTCREATEPEN);
  em_pn.emr.nSize = htofl(sizeof(EMREXTCREATEPEN));

  oldpenhandle++;
  emh_nHandles++;

# ifdef __EMF_DEBUG__
    fprintf(stderr, "Edge %d vis %d type %d width %d color %d \n", 
      oldpenhandle,
      vis, type, width, color);
# endif

  em_pn.ihPen = htofl(oldpenhandle);

  em_pn.offBmi  = htofl(sizeof(EMREXTCREATEPEN));
  em_pn.offBits = htofl(sizeof(EMREXTCREATEPEN));

  if (vis) {	/* Outline is visible */
    em_pn.elp.elpPenStyle = htofl(conv_linetype(type) | PS_GEOMETRIC);
  } else {
    em_pn.elp.elpPenStyle = htofl(PS_NULL | PS_GEOMETRIC);
  }
  em_pn.elp.elpWidth = htofl(width);
  em_pn.elp.elpColor = htofl(conv_color(color));

  emh_write(&em_pn, sizeof(EMREXTCREATEPEN), 1, EMH_RECORD);

  /* Select Object */
  em_so.emr.iType = htofl(EMR_SELECTOBJECT);
  em_so.emr.nSize = htofl(sizeof(EMRSELECTOBJECT));
  em_so.ihObject = htofl(oldpenhandle);
  emh_write(&em_so, sizeof(EMRSELECTOBJECT), 1, EMH_RECORD);
}


/* This procedure turns off the edge visibility. */
static void edgevis(onoff)
  int onoff;
{
  edgeattr(onoff, 0, 0, 0);
}


static void ellipse(e)
  F_ellipse *e;
{
  double x1, y1, x2, y2;
  ellipsetup(e, &x1, &y1, &x2, &y2);
  _ellipse(e->center.x, e->center.y, (int)x1, (int)y1, (int)x2, (int)y2);
}


static void ellipsetup(e, x1, y1, x2, y2)
  F_ellipse *e;
  double *x1, *y1, *x2, *y2;
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


/* Write an enhanced metarecord and keep track of number of bytes written
 * and number of records written in the global header record "emh". */
static size_t emh_write(ptr, size, nmemb, flag)
    const void *ptr;
    size_t	size;
    size_t	nmemb;
    emh_flag	flag;
{
  if (flag == EMH_RECORD) emh_nRecords++;
  emh_nBytes += size * nmemb;
  return( fwrite(ptr, size, nmemb, tfp) );
}


/* Set fill color if standard or user defined color */
static void fillcolr(color)
    int color;
{
  chkcache(color, oldfillcolor);
  _fillcolr(color);
}


/* Used to set solid fill color if something else than standard or user
 * defined color, see fillshade(). */
static void fillcolrgb(r, g, b)
    int r;
    int g;
    int b;
{
  static int oldrgb = UNDEFVALUE;
  int rgb = (r * 256 + g) * 256 + b;
  if (rgb != oldrgb) {
    oldrgb = rgb;
    /*fprintf(tfp, "colrtable %d %d %d %d;\n",
	conv_color(FILL_COLOR_INDEX), r, g, b);*/
    _fillcolr(FILL_COLOR_INDEX);
  } else
    fillcolr(FILL_COLOR_INDEX);
}


/* Computes and sets fill color for solid filled shapes (fill style 0 to 40). */
static void fillshade(l)
    F_line *l;
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
    default:			/* compute shade as a mix of black/white */
	if (l->fill_style == 0) {	/* with fill color */
	    fillcolr(0);
	} else if (l->fill_style < 20) {
	    /*getrgb(l->fill_color, &r, &g, &b);*/
	    f = (float)l->fill_style / 20.;
	    fillcolrgb(round(r * f), round(g * f), round(b * f));
	} else if (l->fill_style == 20) {
	    fillcolr(l->fill_color);
	} else if (l->fill_style < 40) {
	    /*getrgb(l->fill_color, &r, &g, &b);*/
	    f = (float)(l->fill_style - 20) / 20.;
	    fillcolrgb( round(r + f*(255-r)),
	    		round(g + f*(255-g)),
	    		round(b + f*(255-b)) );
	} else if (l->fill_style == 40) {
	    fillcolr(7);
	}
  }
}


static void hatchindex(index)
    int index;
{
  static int oldindex = UNDEFVALUE;

  chkcache(index, oldindex);
  index = conv_pattern_index(index);
  /*fprintf(tfp, "hatchindex %d;\n", index);*/
}


/* Integer cross product */
static int icprod(x1, y1, x2, y2)
    int x1, y1, x2, y2;
{
  return x1 * y2 - y1 * x2;
}


/* Sets EMF interior style */
static void intstyle(style)
    INTSTYLE style;
{
  static INTSTYLE oldstyle = UNDEF;

  chkcache(style, oldstyle);

  switch (style) {
  case HOLLOW:
    /*fprintf(tfp, "intstyle HOLLOW;\n"); */
    break;
  case SOLID:
    /*fprintf(tfp, "intstyle SOLID;\n");*/
    break;
  case HATCH:
    /*fprintf(tfp, "intstyle HATCH;\n");*/
    break;
  case EMPTY:
    /*fprintf(tfp, "intstyle EMPTY;\n");*/
    break;
  default:
    fprintf(stderr, "Unrecognized intstyle %d (program error).\n", style);
  }
}


static void line(p, q)
    F_point *p;
    F_point *q;
{
  _line(p->x, p->y, q->x, q->y);
}


/* This procedure sets the pen line attributes and selects the pen into
 * the device context. */
static void lineattr(type, width, color, join, cap)
  int type;
  int width;
  int color;
  int join;
  int cap;
{
  EMREXTCREATEPEN em_pn;
  EMRSELECTOBJECT em_so;

  memset(&em_pn, 0, sizeof(EMREXTCREATEPEN));
  em_pn.emr.iType = htofl(EMR_EXTCREATEPEN);
  em_pn.emr.nSize = htofl(sizeof(EMREXTCREATEPEN));

  oldpenhandle++;
  emh_nHandles++;

# ifdef __EMF_DEBUG__
    fprintf(stderr, "Pen %d type %d width %d color %d join %d cap %d\n", 
      oldpenhandle,
      type, width, color, join, cap);
# endif

  em_pn.ihPen = htofl(oldpenhandle);

  em_pn.offBmi  = htofl(sizeof(EMREXTCREATEPEN));
  em_pn.offBits = htofl(sizeof(EMREXTCREATEPEN));

  em_pn.elp.elpPenStyle = htofl(conv_linetype(type) | PS_GEOMETRIC |
	conv_capstyle(cap) | conv_joinstyle(join));
  em_pn.elp.elpWidth = htofl(width);
  em_pn.elp.elpColor = htofl(conv_color(color));

  emh_write(&em_pn, sizeof(EMREXTCREATEPEN), 1, EMH_RECORD);

  /* Select Object */
  em_so.emr.iType = htofl(EMR_SELECTOBJECT);
  em_so.emr.nSize = htofl(sizeof(EMRSELECTOBJECT));
  em_so.ihObject = htofl(oldpenhandle);
  emh_write(&em_so, sizeof(EMRSELECTOBJECT), 1, EMH_RECORD);
}


static void picbox(l)
    F_line *l;
{
  static int wgiv = 0;
  if (!wgiv) {
    fprintf(stderr,"Warning: pictures not supported in EMF language\n");
    wgiv = 1;
  }
  polyline(l);
}


static void point(p)
    F_point	*p;
{
  _pos(p->x, p->y);
}


/* Draws polygon boundary */
static void polygon(l)
    F_line *l;
{
  F_point *p, *q, p0, pn;
  int count, erase_head=False, erase_tail=False;
  Dir dir;
  double d;
  EMRPOLYGON em_pg;	/* Polygon in little endian format */
  POINTL *aptl;

  RECTL  rclBounds;	/* Bounding box in host format */
  ulong  cptl;		/* Number of points in the array */

  /* Calculate the number of points and the bounding box. */
  if (!l->points) return;
  for (q=p=l->points, cptl=0; p; q=p, p=p->next) {
    if (cptl == 0) {
      rclBounds.left = p->x;
      rclBounds.top  = p->y;
      rclBounds.right  = p->x;
      rclBounds.bottom = p->y;
    } else {
      if (rclBounds.left > p->x) {
        rclBounds.left = p->x;
      } else if (rclBounds.right < p->x) {
        rclBounds.right = p->x;
      }
      if (rclBounds.top > p->y) {
        rclBounds.top = p->y;
      } else if (rclBounds.bottom < p->y) {
        rclBounds.bottom = p->y;
      }
    }
    cptl++;
  }

# ifdef __EMF_DEBUG__
    fprintf(stderr, "Polygon %d rclBounds (ltrb): %d %d %d %d\n", 
	cptl,
	rclBounds.left,  rclBounds.top,
	rclBounds.right, rclBounds.bottom);
# endif

  /* Fill the array with the points of the polygon */
  aptl = malloc(cptl * sizeof(POINTL));
  for (q=p=l->points, count=0; p; q=p, p=p->next, count++) {
    aptl[count].x = htofl(p->x);
    aptl[count].y = htofl(p->y);
  }

  em_pg.emr.iType = htofl(EMR_POLYGON);
  em_pg.emr.nSize = htofl(sizeof(EMRPOLYGON) + cptl * sizeof(POINTL));

  /* Store bounding box in little endian format */
  em_pg.cptl = htofl(cptl);
  em_pg.rclBounds.left = htofl(rclBounds.left);
  em_pg.rclBounds.top  = htofl(rclBounds.top);
  em_pg.rclBounds.right  = htofl(rclBounds.right);
  em_pg.rclBounds.bottom = htofl(rclBounds.bottom);

  emh_write(&em_pg, sizeof(EMRPOLYGON), 1, EMH_RECORD);
  emh_write(aptl, sizeof(POINTL), cptl, EMH_DATA);

  free(aptl);

}/* end polygon(l) */


/* Draws polyline boundary (with arrows if needed) */
static void polyline(l)
    F_line *l;
{
  F_point *p, *q, p0, pn;
  int count, erase_head=False, erase_tail=False;
  Dir dir;
  double d;
  EMRPOLYLINE em_pl;	/* Polyline in little endian format */
  POINTL *aptl;

  RECTL  rclBounds;	/* Bounding box in host format */
  ulong  cptl;		/* Number of points in the array */

  /* Calculate the number of points and the bounding box. */
  if (!l->points) return;
  for (q=p=l->points, cptl=0; p; q=p, p=p->next) {
    if (cptl == 0) {
      rclBounds.left = p->x;
      rclBounds.top  = p->y;
      rclBounds.right  = p->x;
      rclBounds.bottom = p->y;
    } else {
      if (rclBounds.left > p->x) {
        rclBounds.left = p->x;
      } else if (rclBounds.right < p->x) {
        rclBounds.right = p->x;
      }
      if (rclBounds.top > p->y) {
        rclBounds.top = p->y;
      } else if (rclBounds.bottom < p->y) {
        rclBounds.bottom = p->y;
      }
    }
    cptl++;
  }

# ifdef __EMF_DEBUG__
    fprintf(stderr, "Polyline %d rclBounds (ltrb): %d %d %d %d\n", 
	cptl,
	rclBounds.left,  rclBounds.top,
	rclBounds.right, rclBounds.bottom);
# endif

  /* Fill the array with the points of the polyline */
  aptl = malloc(cptl * sizeof(POINTL));
  for (q=p=l->points, count=0; p; q=p, p=p->next, count++) {
    aptl[count].x = htofl(p->x);
    aptl[count].y = htofl(p->y);
  }

  em_pl.emr.iType = htofl(EMR_POLYLINE);
  em_pl.emr.nSize = htofl(sizeof(EMRPOLYLINE) + cptl * sizeof(POINTL));

  /* Store bounding box in little endian format */
  em_pl.cptl = htofl(cptl);
  em_pl.rclBounds.left = htofl(rclBounds.left);
  em_pl.rclBounds.top  = htofl(rclBounds.top);
  em_pl.rclBounds.right  = htofl(rclBounds.right);
  em_pl.rclBounds.bottom = htofl(rclBounds.bottom);

  emh_write(&em_pl, sizeof(EMRPOLYLINE), 1, EMH_RECORD);
  emh_write(aptl, sizeof(POINTL), cptl, EMH_DATA);
  free(aptl);

  if (!l->points->next) {
    if (l->for_arrow || l->back_arrow)
      fprintf(stderr, "Warning: Arrow at zero-length line segment omitted.\n");
    return;
  }		/* At least two different points now */

  for (q=p=l->points, count=0; p; q=p, p=p->next) {
    if (count!=0 && count%5 == 0)

    if (count == 0 && l->back_arrow) {		/* First point with arrow */
      p0 = *p;
      q = p->next;
      erase_head = !polyline_arrow_adjust(&p0, q, l->back_arrow);
      point(erase_head ? p : &p0);
    }
    else if (!p->next && l->for_arrow) {	/* Last point with arrow */
      pn = *p;
      /* q is one but last point */
      erase_tail = !polyline_arrow_adjust(&pn, q, l->for_arrow);
      point(erase_tail ? p : &pn);
    }
    else					/* Normal point */
      point(p);

    count++;
  }

  if (l->back_arrow) {
    p = l->points;
    q = l->points->next;
    if (direction(p, q, &dir, &d)) {
      if (erase_head) {
	F_point q;
	q.x = round(p->x + dir.x * l->thickness/2.);
	q.y = round(p->y + dir.y * l->thickness/2.);
	lineattr(0, l->thickness*3/2, 7, 0, 0);
	line(&q, &p0);
      }
      arrow(p, l->back_arrow, l, &dir);
    }
  }

  if (l->for_arrow) {
    for (q=l->points, p=l->points->next; p->next; q=p, p=p->next) {}
    /* P is the last point, q is the one but last point */
    if (direction(p, q, &dir, &d)) {
      if (erase_tail) {
	F_point q;
	q.x = round(p->x + dir.x * l->thickness/2.);
	q.y = round(p->y + dir.y * l->thickness/2.);
	lineattr(0, l->thickness*3/2, 7, 0, 0);
	line(&pn, &q);
      }
      arrow(p, l->for_arrow, l, &dir);
    }
  }
}/* end polyline(l) */


/* Replaces p by the starting point of the arrow pointing from q to p.
 * returns True if the arrow is shorter than the line segment [qp].
 * In this case, we shorten the polyline in order to achieve a better
 * fit with the arrow.  If the arrow is longer than the line segment,
 * shortening the segment wont help, and the polyline needs to be erased
 * under the arrow after drawing the polyline and before drawing the
 * arrow. */
static int polyline_arrow_adjust(p, q, a)
    F_point *p; F_point *q; F_arrow *a;
{
  double d, l; Dir dir;

  if (direction(p, q, &dir, &d)) {
    l = arrow_length(a);
    p->x = round(p->x - l * dir.x);
    p->y = round(p->y - l * dir.y);
    return (l < d);
  }
  fprintf(stderr, "Warning: Arrow at zero-length line segment omitted.\n");
  return True;
}


static void pos(p)
    F_pos	*p;
{
  _pos(p->x, p->y);
}


/* Copies coordinates of pos p to point P. */
static void pos2point(P, p)
    F_point *P;
    F_pos *p;
{
  P->x = p->x; P->y = p->y;
}


static void rect(l)
    F_line *l;
{
  EMRRECTANGLE em_rt;

  if (!l->points || !l->points->next || !l->points->next->next) {
    fprintf(stderr, "Warning: Invalid fig box omitted.\n");
    return;
  }

  em_rt.emr.iType = htofl(EMR_RECTANGLE);
  em_rt.emr.nSize = htofl(sizeof(EMRRECTANGLE));
  em_rt.rclBox.left = htofl(l->points->x);
  em_rt.rclBox.top  = htofl(l->points->y);
  em_rt.rclBox.right  = htofl(l->points->next->next->x);
  em_rt.rclBox.bottom = htofl(l->points->next->next->y);

# ifdef __EMF_DEBUG__
    fprintf(stderr, "Rectangle rclBox (ltrb): %d %d %d %d\n", 
	ftohl(em_rt.rclBox.left), ftohl(em_rt.rclBox.top),
	ftohl(em_rt.rclBox.right), ftohl(em_rt.rclBox.bottom));
# endif

  emh_write(&em_rt, sizeof(EMRRECTANGLE), 1, EMH_RECORD);
}


/* Rotates counter clockwise around origin */
static void rotate(x, y, angle)
    double *x, *y, angle;
{
  double s = sin(angle), c = cos(angle), xt = *x, yt = *y;
  *x =  c * xt + s * yt;
  *y = -s * xt + c * yt;
}


/* Draws interior and outline of a simple closed shape.  Cannot be 
 * used for polylines and arcs (open shapes) or for arcboxes
 * (closed but not a simple emf primitive). */
static void shape(l, drawshape)
    F_line *l;
    void (*drawshape)(F_line *);
{
  int style = l->fill_style;

  if (style < 0) {			/* Unfilled shape */
    intstyle(EMPTY);
    edgeattr(1, l->style, l->thickness, l->pen_color);
    drawshape(l);
    return;
  }

  intstyle(SOLID);
  if (style >= 0 && style <= 40) { 	/* Solid filled shape */
    fillshade(l);
    edgeattr(1, l->style, l->thickness, l->pen_color);
  } else {		   		/* Pattern filled shape: draw a */
    fillcolr(l->fill_color);		/* First time for correct pattern */
    edgevis(0);				/* Background color. */
  }
  drawshape(l);

  if (style > 40) {			/* Pattern filled shape */
    intstyle(HATCH);
    hatchindex(style-41);
    fillcolr(l->pen_color);
    edgeattr(1, l->style, l->thickness, l->pen_color);
    drawshape(l);
  }
}


/* Draws interior of a closed shape, taking into account fill color
 * and pattern.  Boundary must be drawn separately.  Used for
 * polylines, arcboxes and arcs. */
static void shape_interior(l, drawshape)
    F_line *l;
    void (*drawshape)(F_line *);
{
  int style = l->fill_style;
  if (style < 0)
    return;

  edgevis(0);				/* Don't draw edges */

  intstyle(SOLID);
  if (style >= 0 && style <= 40)  	/* Solid filled shape */
    fillshade(l);
  else					/* Fill pattern background */
    fillcolr(l->fill_color);
  drawshape(l);

  if (style > 40) {			/* Pattern filled shape */
    intstyle(HATCH);
    hatchindex(style-41);
    fillcolr(l->pen_color);
    drawshape(l);
  }
}


static void text(x, y, h, l, text, type)
    int     x, y;
    double  h, l;			/* Pixels (1200 dpi) */
    char   *text;
    int     type;
{
  int    i, n_chars, n_unicode;
  short	 *utext = NULL;
  EMREXTTEXTOUTW em_tx;		/* Text structure in little endian format */

  memset(&em_tx, 0, sizeof(EMREXTTEXTOUTW));
  em_tx.emr.iType = htofl(EMR_EXTTEXTOUTW);

  switch (type) {
  case T_LEFT_JUSTIFIED:
    em_tx.rclBounds.left = htofl(x);
    em_tx.rclBounds.top  = htofl(y - h);
    em_tx.rclBounds.right  = htofl(x + l);
    em_tx.rclBounds.bottom = htofl(y);
    break;
  case T_CENTER_JUSTIFIED:
    em_tx.rclBounds.left = htofl(x - l/2);
    em_tx.rclBounds.top  = htofl(y - h);
    em_tx.rclBounds.right  = htofl(x + l/2);
    em_tx.rclBounds.bottom = htofl(y);
    break;
  case T_RIGHT_JUSTIFIED:
    em_tx.rclBounds.left = htofl(x - l);
    em_tx.rclBounds.top  = htofl(y - h);
    em_tx.rclBounds.right  = htofl(x);
    em_tx.rclBounds.bottom = htofl(y);
    break;
  default:
    fprintf(stderr, "Unsupported fig text type %d.\n", type);
  }

# ifdef __EMF_DEBUG__
    fprintf(stderr, "Textout |%s| rclBounds (ltrb): %d %d %d %d\n", 
	text,
	ftohl(em_tx.rclBounds.left),  ftohl(em_tx.rclBounds.top),
	ftohl(em_tx.rclBounds.right), ftohl(em_tx.rclBounds.bottom));
# endif

  em_tx.iGraphicsMode = htofl(GM_COMPATIBLE);
  em_tx.exScale = htofl(PIXEL_01MM);
  em_tx.eyScale = htofl(PIXEL_01MM);

  em_tx.emrtext.ptlReference.x = em_tx.rclBounds.left;
  em_tx.emrtext.ptlReference.y = em_tx.rclBounds.top;

  textunicode(text, &n_chars, &utext, &n_unicode);

  em_tx.emrtext.nChars = htofl(n_chars);
  em_tx.emrtext.offString = htofl(sizeof(EMREXTTEXTOUTW));

  em_tx.emr.nSize = htofl(sizeof(EMREXTTEXTOUTW) + n_unicode*sizeof(short));

  emh_write(&em_tx, sizeof(EMREXTTEXTOUTW), 1, EMH_RECORD);
  emh_write(utext, sizeof(short), n_unicode, EMH_DATA);

  free(utext);
}


static void textangle(angle)
    double angle;
{
  int c, s;
  static double oldangle = UNDEFVALUE;
  chkcache(angle, oldangle);
  c = round(1200*cos(angle)); s = round(1200*sin(angle));
  /*fprintf(tfp, "charori (%d,%d) (%d,%d);\n", -s, c, c, s);*/
}


static void textcolr(color)
    int color;
{
  static int oldcolor = UNDEFVALUE;
  chkcache(color, oldcolor);
  color = conv_color(color);
  /*fprintf(tfp, "textcolr %d;\n", color);*/
}


static void textfont(font, flags, size)
  int     font;
  int     flags;
  double  size;
{
  static int oldfont = UNDEFVALUE;
  static int oldfonthandle = 0;
  EMREXTCREATEFONTINDIRECTW em_fn;
  EMRSELECTOBJECT em_so;
  short *utext;
  int    n_chars, n_unicode;

  memset(&em_fn, 0, sizeof(EMREXTCREATEFONTINDIRECTW));
  em_fn.emr.iType = htofl(EMR_EXTCREATEFONTINDIRECTW);
  em_fn.emr.nSize = htofl(sizeof(EMREXTCREATEFONTINDIRECTW));

  font = conv_fontindex(font, flags);		/* Convert the font index */

  oldfonthandle++;
  emh_nHandles++;

  em_fn.ihFont = htofl(oldfonthandle);

  utext = &(em_fn.elfw.elfLogFont.lfFaceName[0]);
  textunicode(lfFaceName[font], &n_chars, &utext, &n_unicode);

  em_fn.elfw.elfLogFont.lfHeight = htofl(-(long)(size * (1200/72.27) + 0.5));
  em_fn.elfw.elfLogFont.lfWeight = htofl(lfWeight[font]);
  em_fn.elfw.elfLogFont.lfItalic = /*uchar*/lfItalic[font];
  em_fn.elfw.elfLogFont.lfCharSet = /*uchar*/lfCharSet[font];
  em_fn.elfw.elfLogFont.lfPitchAndFamily = /*uchar*/lfPitchAndFamily[font];

  emh_write(&em_fn, sizeof(EMREXTCREATEFONTINDIRECTW), 1, EMH_RECORD);

# ifdef __EMF_DEBUG__
    fprintf(stderr,"Textfont: %s  Size: %d  Weight: %d  Italic: %d\n", 
	lfFaceName[font], -ftohl(em_fn.elfw.elfLogFont.lfHeight),
	lfWeight[font], lfItalic[font]);
# endif

  em_so.emr.iType = htofl(EMR_SELECTOBJECT);
  em_so.emr.nSize = htofl(sizeof(EMRSELECTOBJECT));
  em_so.ihObject = htofl(oldfonthandle);
  emh_write(&em_so, sizeof(EMRSELECTOBJECT), 1, EMH_RECORD);
}


static void texttype(type)
    int type;
{
  static int oldtype = UNDEFVALUE;
  chkcache(type, oldtype);
  switch (type) {
  case T_LEFT_JUSTIFIED:
    /*fprintf(tfp, "textalign left base 0.0 0.0;\n");*/
    break;
  case T_CENTER_JUSTIFIED:
    /*fprintf(tfp, "textalign ctr base 0.0 0.0;\n");*/
    break;
  case T_RIGHT_JUSTIFIED:
    /*fprintf(tfp, "textalign right base 0.0 0.0;\n");*/
    break;
  default:
    fprintf(stderr, "Unsupported FIG text type %d.\n", type);
  }
}

/* Converts regular strings to unicode strings.  If the utext pointer is
 * null, memory is allocated.  Note, that carriage returns are converted
 * to nulls. */
static void textunicode(text, n_chars, utext, n_unicode)
  char   *text;
  int    *n_chars;
  short  **utext;		/* If *utext is null, memory is allocated */
  int    *n_unicode;
{
  int    i;
  *n_chars = strlen(text);
  *n_unicode = (*n_chars+2) & (~0x1);		/* Round up and make it even. */
  if (*utext == NULL) {
    *utext = (short *)calloc(*n_unicode, sizeof(short));
  }
  for (i=0; i<*n_chars; i++) {			/* Convert to unicode. */
    if (text[i] == '\n') {
      (*utext)[i] = 0;
    } else {
      (*utext)[i] = htofs(text[i]);
    }
  }
}


static void translate(x, y, tx, ty)
    double *x, *y, tx, ty;
{
  *x += tx; *y += ty;
}


/*~~~~~|><|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void genemf_option(opt, optarg)
   char		 opt;
   char		*optarg;
{
    rounded_arrows = False;
    switch (opt) {
	case 'r': 
	    rounded_arrows = True;
	    break;

	case 'f':	/* Ignore magnification, font sizes and lang here */
	case 'm':
	case 's':
	case 'L':
	    break;

	default:
	    /* other EMF driver options to consider are:
	     * faithful reproduction of FIG linestyles and fill patterns
	     * (linetyles e.g. by drawing multiple short lines), other
	     * EMF encodings beside clear text, non-white (e.g. black) 
	     * background with corresponding change of foreground color, ...
	     */

	    put_msg(Err_badarg, opt, "emf");
	    exit(1);
    }
}


/*~~~~~|><|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void genemf_start(objects)
   F_compound	*objects;
{
  EMRSETMAPMODE em_sm;
  EMRSETVIEWPORTEXTEX em_vp;
  char  *emh_description;		/* Regular text description. */
  short *uni_description = NULL;	/* Unitext discription. */
  int i, n_chars, n_unicode;

  memset(&emh, 0, sizeof(ENHMETAHEADER));
  emh.iType = htofl(EMR_HEADER);

  emh.dSignature = htofl(ENHMETA_SIGNATURE);
  emh.nVersion = htofl(ENHMETA_VERSION);
  emh_nHandles = 1;
  oldpenhandle = 0;			/* Initialize the last pen handle */

  /* Create a description string. */

  emh_description = malloc(80+strlen(from)+strlen(VERSION)+strlen(PATCHLEVEL));
  sprintf(emh_description,
	"Converted from %s using fig2dev -Lemf \n Version %s Patchlevel %s\n",
	from, VERSION, PATCHLEVEL);

  textunicode(emh_description, &n_chars, &uni_description, &n_unicode);

  emh.nDescription = htofl(n_chars);
  emh.offDescription = htofl(sizeof(ENHMETAHEADER));

# ifdef __EMF_DEBUG__
    fprintf(stderr, emh_description);
# endif

  /* The size of the reference device in pixels.  We will define a pixel
   * in xfig uints of 1200 dots per inch and we'll use an 250 x 200 mm
   * or 10.24 x 8.46 inch viewing area. */

  emh.szlMillimeters.cx = htofl(260);		/* 10.24 inches wide. */
  emh.szlMillimeters.cy = htofl(215);		/*  8.46 inches high. */

  emh.szlDevice.cx = htofl(floor(260 * 47.244094 + 0.5));
  emh.szlDevice.cy = htofl(floor(215 * 47.244094 + 0.5));

  /* Incluseive-inclusive bounds in device units (1200 dpi) and
   * 0.01 mm units.  Add 1/20 inch all the way around. */

  emh.rclBounds.left   = htofl(llx - 48);
  emh.rclBounds.top    = htofl(lly - 48);
  emh.rclBounds.right  = htofl(urx + 48);
  emh.rclBounds.bottom = htofl(ury + 48);

  emh.rclFrame.left   = htofl(floor((llx - 48) * PIXEL_01MM));
  emh.rclFrame.top    = htofl(floor((lly - 48) * PIXEL_01MM));
  emh.rclFrame.right  = htofl( ceil((urx + 48) * PIXEL_01MM));
  emh.rclFrame.bottom = htofl( ceil((ury + 48) * PIXEL_01MM));

# ifdef __EMF_DEBUG__
    fprintf(stderr, "rclBounds (ltrb): %d %d %d %d\n",
	ftohl(emh.rclBounds.left),  ftohl(emh.rclBounds.top),
	ftohl(emh.rclBounds.right), ftohl(emh.rclBounds.bottom));
    fprintf(stderr, "rclFrame  (ltrb): %d %d %d %d\n",
	ftohl(emh.rclFrame.left),  ftohl(emh.rclFrame.top),
	ftohl(emh.rclFrame.right), ftohl(emh.rclFrame.bottom));
# endif

  emh.nSize = htofl(sizeof(ENHMETAHEADER) + n_unicode*sizeof(short));

  emh_write(&emh, sizeof(ENHMETAHEADER), 1, EMH_RECORD);
  emh_write(uni_description, sizeof(short), n_unicode, EMH_DATA);

  /* The SetMapMode function sets the mapping mode of the specified
   * device context.  The mapping mode defines the unit of measure used
   * to transform page-space units into device-space units, and also
   * defines the orientation of the device's x and y axes. */
  em_sm.emr.iType = htofl(EMR_SETMAPMODE);
  em_sm.emr.nSize = htofl(sizeof(EMRSETMAPMODE));

  /* MM_TEXT Each logical unit is mapped to one device pixel.  Positive
   * x is to the right; positive y is down.  The MM_TEXT mode allows
   * applications to work in device pixels, whose size varies from device
   * to device. */
  em_sm.iMode = htofl(MM_TEXT);
  emh_write(&em_sm, sizeof(EMRSETMAPMODE), 1, EMH_RECORD);

  free(emh_description);
  free(uni_description);

}


/*~~~~~|><|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void genemf_arc(_a)
    F_arc *_a;
{
    F_arc a = *_a;

    /* print any comments prefixed with "%" */
    print_comments("% ",a.comments, " %");

    /*fprintf(tfp,"%% Arc %%\n");*/
    if (cwarc(&a)) 
	arc_reverse(&a);	/* make counter clockwise arc */

    shape_interior((F_line *)&a, (void (*)(F_line *))arcinterior);
    if (a.thickness > 0) {
	lineattr(a.style, a.thickness, a.pen_color, 0, 0);
	arcoutline(&a);
    }
}


/*~~~~~|><|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void genemf_ellipse(e)
    F_ellipse *e;
{
  /* print any comments prefixed with "%" */
  print_comments("% ",e->comments, " %");

  switch (e->type) {
    case T_ELLIPSE_BY_RAD:
    case T_ELLIPSE_BY_DIA:
	/*fprintf(tfp,"%% Ellipse %%\n");*/
	shape((F_line *)e, (void (*)(F_line *))ellipse);
	break;
    case T_CIRCLE_BY_RAD:
    case T_CIRCLE_BY_DIA:
	/*fprintf(tfp,"%% Circle %%\n");*/
	shape((F_line *)e, (void (*)(F_line *))circle);
	break;
    default:
	fprintf(stderr, "Unsupported FIG ellipse type %d.\n", e->type);
  }
}


/*~~~~~|><|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void genemf_line(l)
    F_line *l;
{
  /* print any comments prefixed with "%" */
  print_comments("% ",l->comments, " %");

  switch (l->type) {
    case T_POLYLINE:
	/*fprintf(tfp,"%% Polyline %%\n");*/
	shape_interior(l, polygon);		/* draw interior */
	lineattr(l->style, l->thickness, l->pen_color,
		 l->join_style, l->cap_style);
	polyline(l);			/* draw boundary */
	break;
    case T_BOX:
	/*fprintf(tfp,"%% Box %%\n");*/
	shape(l, rect);			/* simple closed shape */
	break;
    case T_POLYGON:
	/*fprintf(tfp,"%% Polygon %%\n");*/
	shape(l, polygon);
	break;
    case T_ARC_BOX:
	/*fprintf(tfp,"%% Arc Box %%\n");*/
	shape_interior(l, arcboxinterior);
	lineattr(l->style, l->thickness, l->pen_color,
		 l->join_style, l->cap_style);
	arcboxoutline(l);
	break;
    case T_PIC_BOX:
	/*fprintf(tfp,"%% Imported Picture %%\n");*/
	picbox(l);
	break;
    default:
	fprintf(stderr, "Unsupported FIG polyline type %d.\n", l->type);
	return;
    }
}


/*~~~~~|><|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void genemf_spline(s)
    F_spline *s;
{
  static int wgiv = 0;
  if (!wgiv) {
    fprintf(stderr, "\
Warning: the EMF driver doesn't support (old style) FIG splines.\n\
Suggestion: convert your (old?) FIG image by loading it into xfig v3.2 or\n\
or higher and saving again.\n");
    wgiv = 1;
  }
}


/*~~~~~|><|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void genemf_text(t)
    F_text *t;
{
  /* print any comments prefixed with "%" */
  print_comments("% ",t->comments, " %");

  textfont(t->font, t->flags, t->size);
  texttype(t->type);
  textcolr(t->color);
  textangle(t->angle);
  text(t->base_x, t->base_y, t->height, t->length, t->cstring, t->type);
}


/*~~~~~|><|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
int genemf_end()
{
  EMREOF em_eof;

  memset( &em_eof, 0, sizeof(EMREOF) );
  em_eof.emr.iType = htofl(EMR_EOF);
  em_eof.emr.nSize = htofl(sizeof(EMREOF));
  em_eof.offPalEntries = htofl(sizeof(EMREOF) - sizeof(long));
  em_eof.nSizeLast = htofl(sizeof(EMREOF));
  emh_write(&em_eof, sizeof(EMREOF), 1, EMH_RECORD);

# ifdef __EMF_DEBUG__
    fprintf(stderr, "Metafile Bytes: %d    Records: %d    Handles: %d\n",
	emh_nBytes, emh_nRecords, emh_nHandles);
# endif

  /* Rewrite the updated header record at the beginning of the file */

  emh.nBytes   = htofl(emh_nBytes);
  emh.nRecords = htofl(emh_nRecords);
  emh.nHandles = htofs(emh_nHandles);

  fseek(tfp, 0, SEEK_SET);
  fwrite(&emh, 1, sizeof(ENHMETAHEADER), tfp);
  fseek(tfp, 0, SEEK_END);

  /* all ok */
  return 0;
}


/*~~~~~|><|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
struct driver dev_emf = {
     	genemf_option,
	genemf_start,
	gendev_null,
	genemf_arc,
	genemf_ellipse,
	genemf_line,
	genemf_spline,
	genemf_text,
	genemf_end,
	INCLUDE_TEXT
};
