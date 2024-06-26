/*
 * Fig2dev: Translate Fig code to various Devices
 * Copyright (c) 1985 Supoj Sutanthavibul
 * Copyright (c) 1991 Micah Beck
 * Parts Copyright (c) 1989-2015 by Brian V. Smith
 * Parts Copyright (c) 2015-2023 Thomas Loimer
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
 * Changes:
 *
 * by Thomas Loimer <thomas.loimer@tuwien.ac.at>
 *
 * 2016-12-10
 *	- Make clip area contain the entire arrow head. Then, output drivers
 *	  capable of clipping, e.g., gensvg.c, can produce real hollow
 *	  arrowheads, instead of filling them with white.
 *	- Compute arrow points in floating point numbers, rounding only
 *	  when assigning to the integer arrays. Round halfway cases for both
 *	  positive and negative numbers towards positive infinity.
 *	  This effectively reverts the change from 2015-12-01.
 * 2015-12-25
 *	- Convert function definitions and declarations to prototype form.
 * 2015-12-01
 *	- Round rotated endpoint of arrow, do not add 0.5 (line 890).
 *	- Modify to build with autoconf.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pi.h"

#include "fig2dev.h"	/* includes "bool.h" */
#include "object.h"
#include "bound.h"
#include "localmath.h"

#undef M_PI_2
#undef M_2PI
#define	M_PI_2	1.57079632679489661923
#define	M_2PI	6.28318530717958647692

#define		Two_seventy_deg		(M_PI + M_PI_2)
#define		half(z1 ,z2)		((z1+z2)/2.0)
#define		max(a, b)		(((a) > (b)) ? (a) : (b))
#define		min(a, b)		(((a) < (b)) ? (a) : (b))

static void	arrow_bound(int objtype, F_line *obj,
			int *xmin, int *ymin, int *xmax, int *ymax);
static void	points_bound(F_point *points,
			int *xmin, int *ymin, int *xmax, int *ymax);

/************** ARRAY FOR ARROW SHAPES **************/
/* struct _arrow_shape  */
/* numpts	*/	/* number of points in arrowhead */
/* tipno	*/	/* which point contains the tip */
/* numfillpts	*/	/* number of points to fill */
/* startclip	*/	/* point at which clip region starts */
/* simplefill	*/	/* if true, use points array to fill otherwise
			   use fill_points array */
/* half		*/	/* if true, arrowhead is half-wide and must be
			   shifted to cover the line */
/* tipmv	*/	/* acuteness of tip (smaller angle, larger tipmv) */
/* points[6]	*/	/* points in arrowhead */
/* fillpoints[6]*/	/* points to fill if not "simple" */
struct _arrow_shape arrow_shapes[NUMARROWS] = {	/* NUMARROWS def'd in fig2dev.h */
	/* number of points, index of tip, {datapairs} */
	/* last point must be upper-left point of tail */
	/* type 0 */
	{ 3, 1, 0, 0, true, false, 2.15, {{-1,-0.5}, {0,0}, {-1,0.5}}, {{0,0}}},
	/* place holder for what would be type 0 filled */
	/* Make this a copy of the default arrow above. Maliciously crafted
	   fig files might choose this arrow, hence provide a valid shape. */
	{ 3, 1, 0, 0, true, false, 2.15, {{-1,-0.5}, {0,0}, {-1,0.5}}, {{0,0}}},
	/* type 1a simple triangle */
	{ 4, 1, 0, 2, true, false, 2.1,
		{{-1.0,0.5}, {0,0}, {-1.0,-0.5}, {-1.0,0.5}}, {{0,0}}},
	/* type 1b filled simple triangle*/
	{ 4, 1, 0, 2, true, false, 2.1,
		{{-1.0,0.5}, {0,0}, {-1.0,-0.5}, {-1.0,0.5}}, {{0,0}}},
	/* type 2a concave spearhead */
	{ 5, 1, 0, 2, true, false, 2.6,
		{{-1.25,0.5},{0,0},{-1.25,-0.5},{-1.0,0},{-1.25,0.5}}, {{0,0}}},
	/* type 2b filled concave spearhead */
	{ 5, 1, 0, 2, true, false, 2.6,
		{{-1.25,0.5},{0,0},{-1.25,-0.5},{-1.0,0},{-1.25,0.5}}, {{0,0}}},
	/* type 3a convex spearhead */
	{ 5, 1, 0, 2, true, false, 1.5,
		{{-0.75,0.5},{0,0},{-0.75,-0.5},{-1.0,0},{-0.75,0.5}}, {{0,0}}},
	/* type 3b filled convex spearhead */
	{ 5, 1, 0, 2, true, false, 1.5,
		{{-0.75,0.5},{0,0},{-0.75,-0.5},{-1.0,0},{-0.75,0.5}}, {{0,0}}},
	/* type 4a diamond */
	{ 5, 1, 0, 2, true, false, 1.15,
		{{-0.5,0.5},{0,0},{-0.5,-0.5},{-1.0,0},{-0.5,0.5}}, {{0,0}}},
	/* type 4b filled diamond */
	{ 5, 1, 0, 2, true, false, 1.15,
		{{-0.5,0.5},{0,0},{-0.5,-0.5},{-1.0,0},{-0.5,0.5}}, {{0,0}}},
	/* type 5a/b circle - handled in code */
	{ 0, 0, 0, 0, true, false, 0.0, {{0,0}}, {{0,0}} },
	{ 0, 0, 0, 0, true, false, 0.0, {{0,0}}, {{0,0}} },
	/* type 6a/b half circle - handled in code */
	{ 0, 0, 0, 0, true, false, -1.0, {{0,0}}, {{0,0}} },
	{ 0, 0, 0, 0, true, false, -1.0, {{0,0}}, {{0,0}} },
	/* type 7a square */
	{ 5, 1, 0, 3, true, false, 0.0,
		{{-1.0,0.5},{0,0.5},{0,-0.5},{-1.0,-0.5},{-1.0,0.5}}, {{0,0}}},
	/* type 7b filled square */
	{ 5, 1, 0, 3, true, false, 0.0,
		{{-1.0,0.5},{0,0.5},{0,-0.5},{-1.0,-0.5},{-1.0,0.5}}, {{0,0}}},
	/* type 8a reverse triangle */
	{ 4, 1, 0, 1, true, false, 0.0,
		{{0,0.5},{0,-0.5},{-1.0,0},{0,0.5}}, {{0,0}}},
	/* type 8b filled reverse triangle */
	{ 4, 1, 0, 1, true, false, 0.0,
		{{0,0.5},{0,-0.5},{-1.0,0},{0,0.5}}, {{0,0}}},
	/* type 9a top-half filled concave spearhead */
	{ 5, 1, 3, 2, false, false, 2.6,
		{{-1.25,0.5},{0,0},{-1.25,-0.5},{-1.0,0},{-1.25,0.5}},
		{{-1.25,-0.5},{0,0},{-1,0}}},
	/* type 9b bottom-half filled concave spearhead */
	{ 5, 1, 3, 2, false, false, 2.6,
		{{-1.25,0.5},{0,0},{-1.25,-0.5},{-1.0,0},{-1.25,0.5}},
		{{-1.25,0.5},{0,0},{-1,0}}},
	/* type 10o top-half simple triangle */
	{ 4, 1, 0, 2, true, true, 2.5,
		{{-1.0,0.5}, {0,0}, {-1,0.0}, {-1.0,0.5}}, {{0,0}}},
	/* type 10f top-half filled simple triangle*/
	{ 4, 1, 0, 2, true, true, 2.5,
		{{-1.0,0.5}, {0,0}, {-1,0.0}, {-1.0,0.5}}, {{0,0}}},
	/* type 11o top-half concave spearhead */
	{ 4, 1, 0, 2, true, true, 3.5,
		{{-1.25,0.5}, {0,0}, {-1,0}, {-1.25,0.5}}, {{0,0}}},
	/* type 11f top-half filled concave spearhead */
	{ 4, 1, 0, 2, true, true, 3.5,
		{{-1.25,0.5}, {0,0}, {-1,0}, {-1.25,0.5}}, {{0,0}}},
	/* type 12o top-half convex spearhead */
	{ 4, 1, 0, 2, true, true, 2.5,
		{{-0.75,0.5}, {0,0}, {-1,0}, {-0.75,0.5}}, {{0,0}}},
	/* type 12f top-half filled convex spearhead */
	{ 4, 1, 0, 2, true, true, 2.5,
		{{-0.75,0.5}, {0,0}, {-1,0}, {-0.75,0.5}}, {{0,0}}},
	/* type 13a "wye" */
	{ 3, 0, 0, 0, true, false, -1.0, {{0,-0.5},{-1.0,0},{0,0.5}}, {{0,0}}},
	/* type 13b bar */
	{ 2, 1, 0, 0, true, false, 0.0, {{0,-0.5},{0,0.5}}, {{0,0}}},
	/* type 14a two-prong fork */
	{ 4, 0, 0, 0, true, false, -1.0,
		{{0,-0.5},{-1.0,-0.5},{-1.0,0.5},{0,0.5}}, {{0,0}}},
	/* type 14b backward two-prong fork */
	{ 4, 1, 0, 0, true, false, 0.0,
		{{-1.0,-0.5,},{0,-0.5},{0,0.5},{-1.0,0.5}}, {{0,0}}}
};

void
arc_bound(F_arc *arc, int *xmin, int *ymin, int *xmax, int *ymax)
{
	double	alpha, beta;
	double	dx, dy, radius;
	int	bx, by, sx, sy;

	dx = arc->point[0].x - arc->center.x;
	dy = arc->center.y - arc->point[0].y;
	alpha = atan2(dy, dx);
	if (alpha < 0.0) alpha += M_2PI;

	radius = sqrt(dx*dx + dy*dy);

	dx = arc->point[2].x - arc->center.x;
	dy = arc->center.y - arc->point[2].y;
	beta = atan2(dy, dx);
	if (beta < 0.0) beta += M_2PI;

	bx = max(arc->point[0].x, arc->point[1].x);
	bx = max(arc->point[2].x, bx);
	by = max(arc->point[0].y, arc->point[1].y);
	by = max(arc->point[2].y, by);
	sx = min(arc->point[0].x, arc->point[1].x);
	sx = min(arc->point[2].x, sx);
	sy = min(arc->point[0].y, arc->point[1].y);
	sy = min(arc->point[2].y, sy);

	if (arc->direction == 1) { /* counter clockwise */
		if (alpha > beta) {
			if (alpha <= 0 || 0 <= beta)
				bx = (int)(arc->center.x + radius + 1.0);
			if (alpha <= M_PI_2 || M_PI_2 <= beta)
				sy = (int)(arc->center.y - radius - 1.0);
			if (alpha <= M_PI || M_PI <= beta)
				sx = (int)(arc->center.x - radius - 1.0);
			if (alpha <= Two_seventy_deg || Two_seventy_deg <= beta)
				by = (int)(arc->center.y + radius + 1.0);
		} else {
			if (0 <= beta && alpha <= 0)
				bx = (int)(arc->center.x + radius + 1.0);
			if (M_PI_2 <= beta && alpha <= M_PI_2)
				sy = (int)(arc->center.y - radius - 1.0);
			if (M_PI <= beta && alpha <= M_PI)
				sx = (int)(arc->center.x - radius - 1.0);
			if (Two_seventy_deg <= beta && alpha <= Two_seventy_deg)
				by = (int)(arc->center.y + radius + 1.0);
		}
	} else {	/* clockwise	*/
		if (alpha > beta) {
			if (beta <= 0 && 0 <= alpha)
				bx = (int)(arc->center.x + radius + 1.0);
			if (beta <= M_PI_2 && M_PI_2 <= alpha)
				sy = (int)(arc->center.y - radius - 1.0);
			if (beta <= M_PI && M_PI <= alpha)
				sx = (int)(arc->center.x - radius - 1.0);
			if (beta <= Two_seventy_deg && Two_seventy_deg <= alpha)
				by = (int)(arc->center.y + radius + 1.0);
		} else {
			if (0 <= alpha || beta <= 0)
				bx = (int)(arc->center.x + radius + 1.0);
			if (M_PI_2 <= alpha || beta <= M_PI_2)
				sy = (int)(arc->center.y - radius - 1.0);
			if (M_PI <= alpha || beta <= M_PI)
				sx = (int)(arc->center.x - radius - 1.0);
			if (Two_seventy_deg <= alpha || beta <= Two_seventy_deg)
				by = (int)(arc->center.y + radius + 1.0);
		}
	}
	/* if pie-wedge type, account for the center point */
	if(arc->type == T_PIE_WEDGE_ARC) {
		sx = min((int)arc->center.x, sx);
		bx = max((int)arc->center.x, bx);
		sy = min((int)arc->center.y, sy);
		by = max((int)arc->center.y, by);
	}

	*xmin = sx;
	*ymin = sy;
	*xmax = bx;
	*ymax = by;

	/* now add in the arrow (if any) boundaries */
	arrow_bound(OBJ_ARC, (F_line *)arc, xmin, ymin, xmax, ymax);
}

void
compound_bound(F_compound *compound, int *xmin, int *ymin, int *xmax, int *ymax,
		int include)
{
	F_arc	*a;
	F_ellipse	*e;
	F_spline	*s;
	F_line	*l;
	F_text	*t;
	int		 bx, by, sx, sy, first = 1;
	int		 llx, lly, urx, ury;
	int	         half_wd;

	llx = lly =  10000000;
	urx = ury = -10000000;
	while(compound != NULL) {
		for (a = compound->arcs; a != NULL; a = a->next) {
			if (adjust_boundingbox && !depth_filter(a->depth))
				continue;
			arc_bound(a, &sx, &sy, &bx, &by);
			half_wd = (a->thickness + 1) / 2;
			if (first) {
				first = 0;
				llx = sx - half_wd;
				lly = sy - half_wd;
				urx = bx + half_wd;
				ury = by + half_wd;
			} else {
				llx = min(llx, sx - half_wd);
				lly = min(lly, sy - half_wd);
				urx = max(urx, bx + half_wd);
				ury = max(ury, by + half_wd);
			}
		}

		if (compound->compounds) {
			compound_bound(compound->compounds, &sx, &sy, &bx, &by,
					include);
			if (first) {
				first = 0;
				llx = sx;
				lly = sy;
				urx = bx;
				ury = by;
			} else {
				llx = min(llx, sx);
				lly = min(lly, sy);
				urx = max(urx, bx);
				ury = max(ury, by);
			}
		}

		for (e = compound->ellipses; e != NULL; e = e->next) {
			if (adjust_boundingbox && !depth_filter(e->depth))
				continue;
			ellipse_bound(e, &sx, &sy, &bx, &by);
			if (first) {
				first = 0;
				llx = sx;
				lly = sy;
				urx = bx;
				ury = by;
			} else {
				llx = min(llx, sx);
				lly = min(lly, sy);
				urx = max(urx, bx);
				ury = max(ury, by);
			}
		}

		for (l = compound->lines; l != NULL; l = l->next) {
			if (adjust_boundingbox && !depth_filter(l->depth))
				continue;
			line_bound(l, &sx, &sy, &bx, &by);
			/* pictures have no line thickness */
			if (l->type == T_PIC_BOX)
				half_wd = 0;
			else
				half_wd = ceil((double)(l->thickness+1) /
						sqrt(2.0));
			/* leave space for corners, better approach needs
			   much more math! */
			if (first) {
				first = 0;
				llx = sx - half_wd;
				lly = sy - half_wd;
				urx = bx + half_wd;
				ury = by + half_wd;
			} else {
				llx = min(llx, sx - half_wd);
				lly = min(lly, sy - half_wd);
				urx = max(urx, bx + half_wd);
				ury = max(ury, by + half_wd);
			}
		}

		for (s = compound->splines; s != NULL; s = s->next) {
			if (adjust_boundingbox && !depth_filter(s->depth))
				continue;
			spline_bound(s, &sx, &sy, &bx, &by);
			half_wd = (s->thickness+1) / 2;
			if (first) {
				first = 0;
				llx = sx - half_wd; lly = sy - half_wd;
				urx = bx + half_wd; ury = by + half_wd;
			} else {
				llx = min(llx, sx - half_wd);
				lly = min(lly, sy - half_wd);
				urx = max(urx, bx + half_wd);
				ury = max(ury, by + half_wd);
			}
		}

		for (t = compound->texts; t != NULL; t = t->next) {
			if (adjust_boundingbox && !depth_filter(t->depth))
				continue;
			text_bound(t, &sx, &sy, &bx, &by, include);
			if (first) {
				first = 0;
				llx = sx; lly = sy;
				urx = bx; ury = by;
			} else {
				llx = min(llx, sx); lly = min(lly, sy);
				urx = max(urx, bx); ury = max(ury, by);
			}
		}
		compound = compound->next;
	}

	*xmin = llx; *ymin = lly;
	*xmax = urx; *ymax = ury;
}

void
ellipse_bound(F_ellipse *e, int *xmin, int *ymin, int *xmax, int *ymax)
{
	/* stolen from xfig-2.1.8 MAX from xfig == max here*/

	int	    half_wd;
	double	    c1, c2, c3, c4, c5, c6, v1, cphi, sphi, cphisqr, sphisqr;
	double	    xleft, xright, d, asqr, bsqr;
	int	    yymax, yy=0;
	float	    xcen, ycen, a, b;

	xcen = e->center.x;
	ycen = e->center.y;
	a = e->radiuses.x;
	b = e->radiuses.y;
	if (a==0 || b==0) {
		*xmin = *xmax = xcen;
		*ymin = *ymax = ycen;
		return;
	}

	cphi = cos((double)e->angle);
	sphi = sin((double)e->angle);
	cphisqr = cphi*cphi;
	sphisqr = sphi*sphi;
	asqr = a*a;
	bsqr = b*b;

	c1 = (cphisqr/asqr)+(sphisqr/bsqr);
	c2 = ((cphi*sphi/asqr)-(cphi*sphi/bsqr))/c1;
	c3 = (bsqr*cphisqr) + (asqr*sphisqr);
	yymax = sqrt(c3);
	c4 = a*b/c3;
	c5 = 0;
	v1 = c4*c4;
	c6 = 2*v1;
	c3 = c3*v1-v1;
	/* odd first points */
	*xmin = *ymin =  10000000;
	*xmax = *ymax = -10000000;
	if (yymax % 2) {
		d = sqrt(c3);
		*xmin = min(*xmin,xcen-ceil(d));
		*xmax = max(*xmax,xcen+ceil(d));
		*ymin = min(*ymin,ycen);
		*ymax = max(*ymax,ycen);
		c5 = c2;
		yy=1;
	}
	while (c3>=0) {
		d = sqrt(c3);
		xleft = c5-d;
		xright = c5+d;
		*xmin = min(*xmin,xcen+floor(xleft));
		*xmax = max(*xmax,xcen+ceil(xleft));
		*ymax = max(*ymax,ycen+yy);
		*xmin = min(*xmin,xcen+floor(xright));
		*xmax = max(*xmax,xcen+ceil(xright));
		*ymax = max(*ymax,ycen+yy);
		*xmin = min(*xmin,xcen-ceil(xright));
		*xmax = max(*xmax,xcen-floor(xright));
		*ymin = min(*ymin,ycen-yy);
		*xmin = min(*xmin,xcen-ceil(xleft));
		*xmax = max(*xmax,xcen-floor(xleft));
		*ymin = min(*ymin,ycen-yy);
		c5+=c2;
		v1+=c6;
		c3-=v1;
		yy=yy+1;
	}
	/* for simplicity, just add half the line thickness to xmax and ymax
	   and subtract half from xmin and ymin */
	half_wd = (e->thickness+1)/2; /*correct for integer division */
	*xmax += half_wd;
	*ymax += half_wd;
	*xmin -= half_wd;
	*ymin -= half_wd;
}

void
line_bound(F_line *l, int *xmin, int *ymin, int *xmax, int *ymax)
{
	points_bound(l->points, xmin, ymin, xmax, ymax);
	/* now add in the arrow (if any) boundaries but
	   only if the line has two or more points */
	if (l->points->next)
		arrow_bound(OBJ_POLYLINE, l, xmin, ymin, xmax, ymax);
}

static void
int_spline_bound(F_spline *s, int *xmin, int *ymin, int *xmax, int *ymax)
{
	F_point		*p1, *p2;
	F_control	*cp1, *cp2;
	double		x0, y0, x1, y1, x2, y2, x3, y3, sx1, sy1, sx2, sy2;
	double		tx, ty, tx1, ty1, tx2, ty2;
	double		sx, sy, bx, by;

	p1 = s->points;
	sx = bx = p1->x;
	sy = by = p1->y;
	cp1 = s->controls;
	for (p2 = p1->next, cp2 = cp1->next; p2 != NULL;
			p1 = p2, cp1 = cp2, p2 = p2->next, cp2 = cp2->next) {
		x0 = p1->x; y0 = p1->y;
		x1 = cp1->rx; y1 = cp1->ry;
		x2 = cp2->lx; y2 = cp2->ly;
		x3 = p2->x; y3 = p2->y;
		tx = half(x1, x2); ty = half(y1, y2);
		sx1 = half(x0, x1); sy1 = half(y0, y1);
		sx2 = half(sx1, tx); sy2 = half(sy1, ty);
		tx2 = half(x2, x3); ty2 = half(y2, y3);
		tx1 = half(tx2, tx); ty1 = half(ty2, ty);

		sx = min(x0, sx); sy = min(y0, sy);
		sx = min(sx1, sx); sy = min(sy1, sy);
		sx = min(sx2, sx); sy = min(sy2, sy);
		sx = min(tx1, sx); sy = min(ty1, sy);
		sx = min(tx2, sx); sy = min(ty2, sy);
		sx = min(x3, sx); sy = min(y3, sy);

		bx = max(x0, bx); by = max(y0, by);
		bx = max(sx1, bx); by = max(sy1, by);
		bx = max(sx2, bx); by = max(sy2, by);
		bx = max(tx1, bx); by = max(ty1, by);
		bx = max(tx2, bx); by = max(ty2, by);
		bx = max(x3, bx); by = max(y3, by);
	}
	*xmin = round(sx);
	*ymin = round(sy);
	*xmax = round(bx);
	*ymax = round(by);
}

static void
normal_spline_bound(F_spline *s, int *xmin, int *ymin, int *xmax, int *ymax)
{
	F_point	*p;
	double	cx1, cy1, cx2, cy2, cx3, cy3, cx4, cy4;
	double	x1, y1, x2, y2, sx, sy, bx, by;
	double	px, py, qx, qy;

	p = s->points;
	x1 = p->x;  y1 = p->y;
	p = p->next;
	x2 = p->x;  y2 = p->y;
	cx1 = (x1 + x2) / 2.0;   cy1 = (y1 + y2) / 2.0;
	cx2 = (cx1 + x2) / 2.0;  cy2 = (cy1 + y2) / 2.0;
	if (closed_spline(s)) {
		x1 = (cx1 + x1) / 2.0;
		y1 = (cy1 + y1) / 2.0;
	}
	sx = min(x1, cx2); sy = min(y1, cy2);
	bx = max(x1, cx2); by = max(y1, cy2);

	for (p = p->next; p != NULL; p = p->next) {
		x1 = x2;  y1 = y2;
		x2 = p->x;  y2 = p->y;
		cx4 = (x1 + x2) / 2.0; cy4 = (y1 + y2) / 2.0;
		cx3 = (x1 + cx4) / 2.0; cy3 = (y1 + cy4) / 2.0;
		cx2 = (cx4 + x2) / 2.0;  cy2 = (cy4 + y2) / 2.0;

		px = min(cx2, cx3); py = min(cy2, cy3);
		qx = max(cx2, cx3); qy = max(cy2, cy3);

		sx = min(sx, px); sy = min(sy, py);
		bx = max(bx, qx); by = max(by, qy);
	}
	if (closed_spline(s)) {
		*xmin = floor(sx );
		*ymin = floor(sy );
		*xmax = ceil (bx );
		*ymax = ceil (by );
	} else {
		*xmin = floor(min(sx, x2) );
		*ymin = floor(min(sy, y2) );
		*xmax = ceil (max(bx, x2) );
		*ymax = ceil (max(by, y2) );
	}
}

void
spline_bound(F_spline *s, int *xmin, int *ymin, int *xmax, int *ymax)
{
	if (int_spline(s)) {
		int_spline_bound(s, xmin, ymin, xmax, ymax);
	}
	else {
		normal_spline_bound(s, xmin, ymin, xmax, ymax);
	}
	/* now do any arrows */
	arrow_bound(OBJ_SPLINE, (F_line *)s, xmin, ymin, xmax, ymax);
}

double
rot_x(double x, double y, double angle)
{
	return(x*cos(-angle)-y*sin(-angle));
}

double
rot_y(double x, double y, double angle)
{
	return(x*sin(-angle)+y*cos(-angle));
}


void
text_bound(F_text *t, int *xmin, int *ymin, int *xmax, int *ymax, int inc_text)
{
	double	dx1, dx2, dx3, dx4, dy1, dy2, dy3, dy4;
	int	descend;

	bool	include;

	/*
	 * include text only:
	 *  1. if inc_text is true AND
	 *  2. not special OR is special and contains either
	 *     a "$" (inline equation) or backslash "\"
	 */
	include = (inc_text &&
			((t->flags & SPECIAL_TEXT) == 0 ||
				(!strchr(t->cstring,'\\') &&
				 !strchr(t->cstring,'$'))));
	/* look for descenders in string (this is a kludge - next version
	   of xfig should include ascent/descent in text structure */
	descend = (strchr(t->cstring,'g') || strchr(t->cstring,'j') ||
			strchr(t->cstring,'p') || strchr(t->cstring,'q') ||
			strchr(t->cstring,'y') || strchr(t->cstring,'$') ||
			strchr(t->cstring,'(') || strchr(t->cstring,')') ||
			strchr(t->cstring,'{') || strchr(t->cstring,'}') ||
			strchr(t->cstring,'[') || strchr(t->cstring,']') ||
			strchr(t->cstring,',') || strchr(t->cstring,';') ||
			strchr(t->cstring,'_'));

	/* check if Symbol font with any descenders */
	if (!descend && psfont_text(t) && t->font == 32)
		descend =
			(strchr(t->cstring,'b')		/* beta  */ ||
			 strchr(t->cstring,'c')		/* chi   */ ||
			 strchr(t->cstring,'f')		/* phi   */ ||
			 strchr(t->cstring,'g')		/* gamma */ ||
			 strchr(t->cstring,'h')		/* eta   */ ||
			 strchr(t->cstring,'j')		/* phi1  */ ||
			 strchr(t->cstring,'m')		/* mu    */ ||
			 strchr(t->cstring,'r')		/* rho   */ ||
			 strchr(t->cstring,'x')		/* xi    */ ||
			 strchr(t->cstring,'y')		/* psi   */ ||
			 strchr(t->cstring,'z')		/* zeta  */ ||
			 strchr(t->cstring,'C'+'\200')	/* weierstrass    */ ||
			 strchr(t->cstring,'J'+'\200')	/* reflexsuperset */ ||
			 strchr(t->cstring,'M'+'\200')	/* reflexsubset   */ ||
			 strchr(t->cstring,'U'+'\200')	/* product        */ ||
			 strchr(t->cstring,'a'+'\200')	/* angleleft      */ ||
			 strchr(t->cstring,'e'+'\200')	/* summation      */ ||
			 strchr(t->cstring,'f'+'\200')	/* parenlefttp    */ ||
			 strchr(t->cstring,'h'+'\200')	/* parenleftbt    */ ||
			 strchr(t->cstring,'q'+'\200')	/* angleright     */ ||
			 strchr(t->cstring,'r'+'\200')	/* integral       */ ||
			 strchr(t->cstring,'v'+'\200')	/* parenrighttp   */ ||
			 strchr(t->cstring,'x'+'\200')	/* parenrightbt   */ ||
			 strchr(t->cstring,'&'+'\200')); /* florin        */

	/* characters have some extent downside */
	if (t->type == T_CENTER_JUSTIFIED) {
		dx1 = (include?  (t->length/1.95) : 0.0);    dy1 =  0.0;
		dx2 = (include? -(t->length/1.95) : 0.0);    dy2 =  0.0;
		dx3 = (include?  (t->length/1.95) : 0.0);    dy3 = -t->height;
		dx4 = (include? -(t->length/1.95) : 0.0);    dy4 = -t->height;
	} else if (t->type == T_RIGHT_JUSTIFIED) {
		dx1 = 0.0;				     dy1 =  0.0;
		dx2 = (include? -t->length*1.0256 : 0.0);    dy2 =  0.0;
		dx3 = 0.0;				     dy3 = -t->height;
		dx4 = (include? -t->length*1.0256 : 0.0);    dy4 = -t->height;
	} else {
		dx1 = (include ? t->length*1.0256 : 0.0);    dy1 =  0.0;
		dx2 = 0.0;				     dy2 =  0.0;
		dx3 = (include ? t->length*1.0256 : 0.0);    dy3 = -t->height;
		dx4 = 0.0;				     dy4 = -t->height;
	}
	if (descend) {
		dy1 = 0.3*t->height;
		dy2 = 0.3*t->height;
		dy3 = -0.8*t->height;
		dy4 = -0.8*t->height;
	}

	*xmax = t->base_x +
		max(max(rot_x(dx1,dy1,t->angle), rot_x(dx2,dy2,t->angle)),
			max(rot_x(dx3,dy3,t->angle), rot_x(dx4,dy4,t->angle))) +
		THICK_SCALE;
	*ymax = t->base_y +
		max(max(rot_y(dx1,dy1,t->angle), rot_y(dx2,dy2,t->angle)),
			max(rot_y(dx3,dy3,t->angle), rot_y(dx4,dy4,t->angle))) +
		THICK_SCALE;

	*xmin = t->base_x +
		min(min(rot_x(dx1,dy1,t->angle), rot_x(dx2,dy2,t->angle)),
			min(rot_x(dx3,dy3,t->angle), rot_x(dx4,dy4,t->angle))) -
		THICK_SCALE;
	*ymin = t->base_y +
		min(min(rot_y(dx1,dy1,t->angle), rot_y(dx2,dy2,t->angle)),
			min(rot_y(dx3,dy3,t->angle), rot_y(dx4,dy4,t->angle))) -
		THICK_SCALE;
}

static void
points_bound(F_point *points, int *xmin, int *ymin, int *xmax, int *ymax)
{
	int	bx, by, sx, sy;
	F_point	*p;

	bx = sx = points->x; by = sy = points->y;
	for (p = points->next; p != NULL; p = p->next) {
		sx = min(sx, p->x); sy = min(sy, p->y);
		bx = max(bx, p->x); by = max(by, p->y);
	}
	*xmin = sx; *ymin = sy;
	*xmax = bx; *ymax = by;
}

/*
static void
control_points_bound(F_control *cps, int *xmin, int *ymin, int *xmax, int *ymax)
{
	F_control	*c;
	double		bx, by, sx, sy;

	bx = sx = cps->lx;
	by = sy = cps->ly;
	sx = min(sx, cps->rx); sy = min(sy, cps->ry);
	bx = max(bx, cps->rx); by = max(by, cps->ry);
	for (c = cps->next; c != NULL; c = c->next) {
	    sx = min(sx, c->lx); sy = min(sy, c->ly);
	    bx = max(bx, c->lx); by = max(by, c->ly);
	    sx = min(sx, c->rx); sy = min(sy, c->ry);
	    bx = max(bx, c->rx); by = max(by, c->ry);
	    }
	*xmin = round(sx); *ymin = round(sy);
	*xmax = round(bx); *ymax = round(by);
}
*/

/* extend xmin, ymin xmax, ymax by the arrow boundaries of obj (if any) */

static void
arrow_bound(int objtype, F_line *obj, int *xmin, int *ymin, int *xmax,
		int *ymax)
{
	int	fxmin, fymin, fxmax, fymax;
	int	bxmin, bymin, bxmax, bymax;
	F_point	*p, *q;
	F_arc	*a;
	int	p1x, p1y, p2x, p2y;
	int	dum;
	int	npts, i;
	F_pos	arrowpts[50], arrowdumpts[50];

	if (obj->for_arrow) {
		if (objtype == OBJ_ARC) {
			a = (F_arc *) obj;
			compute_arcarrow_angle(a->center.x, a->center.y,
					(double)a->point[2].x,
					(double)a->point[2].y,
					a->direction, a->for_arrow, &p1x, &p1y);
			p2x = a->point[2].x;	/* forward tip */
			p2y = a->point[2].y;
		} else if (objtype == OBJ_POLYLINE) {
			p1x = obj->last[1].x;
			p1y = obj->last[1].y;
			p2x = obj->last[0].x;
			p2y = obj->last[0].y;
		} else { /* objype == OBJ_SPLINE */
			/* this doesn't work very well for a spline with few
			   points and lots of curvature */

			/* locate last point (forward tip) and
			   next-to-last point */
			for (p = obj->points; p->next; p = p->next)
				q = p;
			p1x = q->x;
			p1y = q->y;
			p2x = p->x;
			p2y = p->y;
		}
		calc_arrow(p1x, p1y, p2x, p2y, obj->thickness, obj->for_arrow,
			arrowpts, &npts, arrowdumpts, &dum, arrowdumpts, &dum);
		fxmin=fymin=10000000;
		fxmax=fymax=-10000000;
		for (i=0; i<npts; i++) {
			fxmin = MIN(fxmin, arrowpts[i].x);
			fymin = MIN(fymin, arrowpts[i].y);
			fxmax = MAX(fxmax, arrowpts[i].x);
			fymax = MAX(fymax, arrowpts[i].y);
		}
		*xmin = MIN(*xmin, fxmin);
		*xmax = MAX(*xmax, fxmax);
		*ymin = MIN(*ymin, fymin);
		*ymax = MAX(*ymax, fymax);
	}
	if (obj->back_arrow) {
		if (objtype == OBJ_ARC) {
			a = (F_arc *) obj;
			compute_arcarrow_angle(a->center.x, a->center.y,
				(double) a->point[0].x, (double) a->point[0].y,
				a->direction ^ 1, a->back_arrow, &p1x, &p1y);
			p2x = a->point[0].x;	/* backward tip */
			p2y = a->point[0].y;
		} else {
			p1x = obj->points->next->x;	/* second point */
			p1y = obj->points->next->y;
			p2x = obj->points->x;	/* first point (forward tip) */
			p2y = obj->points->y;
		}
		calc_arrow(p1x, p1y, p2x, p2y, obj->thickness, obj->back_arrow,
			arrowpts, &npts, arrowdumpts, &dum, arrowdumpts, &dum);
		bxmin=bymin=10000000;
		bxmax=bymax=-10000000;
		for (i=0; i<npts; i++) {
			bxmin = MIN(bxmin, arrowpts[i].x);
			bymin = MIN(bymin, arrowpts[i].y);
			bxmax = MAX(bxmax, arrowpts[i].x);
			bymax = MAX(bymax, arrowpts[i].y);
		}
		*xmin = MIN(*xmin, bxmin);
		*xmax = MAX(*xmax, bxmax);
		*ymin = MIN(*ymin, bymin);
		*ymax = MAX(*ymax, bymax);
	}
}


/****************************************************************

 calc_arrow - calculate arrowhead points heading from (x1, y1) to (x2, y2)

		        |\
		        |  \
		        |    \
(x1,y1) +---------------|      \+ (x2, y2)
		        |      /
		        |    /
		        |  /
		        |/

 Fills points[] array with npoints arrowhead *outline* coordinates and
 fillpoints[] array with nfillpoints points for the part to be filled *IF*
 it is a special arrowhead that has a different fill area than the outline.

 Otherwise, the points[] array is also used to fill the arrowhead in draw_arrow()
 The linethick param is the thickness of the *main line/spline/arc*,
 not the arrowhead.

 The clippts[] array is filled with the clip area so that the line won't
 protrude through the arrowhead.

****************************************************************/

/* All ROT? macros operate on float numbers and their result is assigned
 * to integers. Round both positive and negative values in positive
 * direction.
 */
#define ROTX(x,y)	floor( (x)*cosa + (y)*sina + xa + 0.5)
#define ROTY(x,y)	floor(-(x)*sina + (y)*cosa + ya + 0.5)

#define ROTX2(x,y)	floor( (x)*cosa + (y)*sina + x2 + 0.5)
#define ROTY2(x,y)	floor(-(x)*sina + (y)*cosa + y2 + 0.5)

#define ROTXC(x,y)	floor( (x)*cosa + (y)*sina + fix_x + 0.5)
#define ROTYC(x,y)	floor(-(x)*sina + (y)*cosa + fix_y + 0.5)

#define		THICKNESS(T)	(T <= THICK_SCALE ? 0.5*T : T - THICK_SCALE)

void
calc_arrow(int x1, int y1, int x2, int y2, int linethick, F_arrow *arrow,
		F_pos points[], int *npoints, F_pos fillpoints[],
		int *nfillpoints, F_pos clippts[], int *nclippts)
{
	bool	thickline;
	double	x, y, xb, yb, l, sina, cosa;
	double	mx, my;
	double	ddx, ddy, lpt, tipmv;
	double	miny, maxy;
	double	xa, ya;
	double	wd  = (double) arrow->wid;
	double	len = (double) arrow->ht;
	double	line_thk, thk;
	double	halfthick, clipthick;
	int	dx, dy;
	int	type, style, indx;
	int	i, np;

	line_thk = THICKNESS(linethick);
	/* to enlarge the clip area in case the line is thick */
	halfthick = line_thk / 2.;
	/* Make the clip area slightly larger than the line thickness.
	   With less than 3, the ps-driver produces small shadows. */
	clipthick = halfthick + 3.;

	/* types = 0...10 */
	type = arrow->type;
	/* style = 0 (unfilled) or 1 (filled) */
	style = arrow->style;
	/* index into shape array */
	indx = 2*type + style;

	*npoints = *nfillpoints = 0;
	dx = x2 - x1;
	dy = y1 - y2;
	if (dx==0 && dy==0)
		return;

	/* lpt is the amount the arrowhead extends beyond the end of the
	 * line because of the sharp point (miter join) */
	tipmv = arrow_shapes[indx].tipmv;
	lpt = 0.0;
	thk = THICKNESS(arrow->thickness);
	if (tipmv > 0.0)
		lpt = thk * sqrt(wd*wd + tipmv*tipmv*len*len) / 2. / fabs(wd);
	else if (tipmv == 0.0)
		lpt = thk / 2.0;	/* types which have blunt end */
	/* (Don't adjust those with tipmv < 0) */

	/* alpha is the angle the line is relative to horizontal */
	l = sqrt((double)dx * dx + (double)dy * dy);
	sina = dy / l;
	cosa = dx / l;

	/* ddx, ddy is amount to move end of line back so that arrowhead point
	   ends where line used to */
	ddx = lpt * (-cosa);
	ddy = lpt * sina;

	/* move endpoint of line back */
	mx = x2 + ddx;
	my = y2 + ddy;

	xb = mx * cosa - my * sina;
	yb = mx * sina + my * cosa;

	/* (xa,ya) is the rotated endpoint (used in ROTX and ROTY macros) */
	xa = xb * cosa + yb * sina;
	ya = -xb * sina + yb * cosa;

	miny =  100000.0;
	maxy = -100000.0;

	if (type == 5 || type == 6) {
		/*
		 * CIRCLE and HALF-CIRCLE arrowheads
		 *
		 * We approximate circles with 40 points
		 */
		double	maxx;
		double	fix_x, fix_y, xs, ys;
		double	angle, init_angle, radius, rads;
		int		phase;

		/* use original dx, dy to get starting angle */
		init_angle = atan2(dy, dx);
		if (init_angle < 0.0) init_angle += M_2PI;

		radius = len/2.0;

		/* (xs,ys) is a point the length of the arrowhead BACK from
		 * the end of the shaft */
		/* for the half circle, use 0.0 */
		if (type == 5) {
			l = len;
			maxx = radius;
		} else {
			l = 0.0;
			maxx = 0.;
		}
		xs =  (xb - l) * cosa + yb * sina;
		ys = -(xb - l) * sina + yb * cosa;

		/* calc new (dx, dy) from moved endpoint to (xs, ys) */
		dx = mx - xs;
		dy = my - ys;
		fix_x = xs + (dx / 2.0);
		fix_y = ys + (dy / 2.0);
		/* choose number of points for circle */
		*npoints = np = 40;

		if (type == 5) {
			/* full circle */
			init_angle = 5.0*M_PI_2 - init_angle;
			rads = M_2PI;
		} else {
			/* half circle */
			init_angle = 3.0*M_PI_2 - init_angle;
			rads = M_PI;
		}

		/* draw the half or full circle */
		for (i = 0; i < np; ++i) {
			angle = init_angle - (rads * i / (double)(np - 1));
			x = fix_x + radius * cos(angle);
			points[i].x = floor(x + 0.5);
			y = fix_y + radius * sin(angle);
			points[i].y = floor(y + 0.5);
		}

		/* set clipping to a box at least as large as the line thickness
		 * or diameter of the circle, whichever is larger */
		/* 4 points in clip box */
		thickline = halfthick > radius;

		/* start at first point (half-circle) or half into the circle */
		if (type == 6)
			i = 0;
		else
			i =  np / 2;
		for (phase = i; i < np; ++i) {
			clippts[i - phase].x = points[i].x;
			clippts[i - phase].y = points[i].y;
		}
		i -= phase;
		if (thickline) {
			clippts[i].x = ROTXC(0, clipthick);
			clippts[i].y = ROTYC(0, clipthick);
			++i;
		}
		clippts[i].x = ROTXC(maxx + clipthick, clipthick);
		clippts[i].y = ROTYC(maxx + clipthick, clipthick);
		++i;
		clippts[i].x = ROTXC(maxx + clipthick, -clipthick);
		clippts[i].y = ROTYC(maxx + clipthick, -clipthick);
		++i;
		if (thickline) {
			clippts[i].x = ROTXC(0, -clipthick);
			clippts[i].y = ROTYC(0, -clipthick);
			++i;
		}
		*nclippts = i;

	} else {
		/*
		 * ALL OTHER HEADS
		 */
		double	offset;

		*npoints = arrow_shapes[indx].numpts;
		/*
		 * we'll shift the half arrowheads down by the difference of the
		 * main line thickness and the arrowhead thickness to make it
		 * flush with the main line
		 */
		if (arrow_shapes[indx].half)
			/* offset = (linethick - arrow->thickness)/2; */
			offset = (line_thk - thk) / 2.;
		else
			offset = 0.;

		/* fill the points array with the outline */
		for (i = 0; i < *npoints; ++i) {
			x = arrow_shapes[indx].points[i].x * len;
			y = arrow_shapes[indx].points[i].y * wd - offset;
			miny = MIN(y, miny);
			maxy = MAX(y, maxy);
			points[i].x = ROTX(x,y);
			points[i].y = ROTY(x,y);
		}

		/* and the fill points array if there are fill points different
		 * from the outline */
		*nfillpoints = arrow_shapes[indx].numfillpts;
		for (i = 0; i < *nfillpoints; ++i) {
			x = arrow_shapes[indx].fillpoints[i].x * len;
			y = arrow_shapes[indx].fillpoints[i].y * wd - offset;
			miny = MIN(y, miny);
			maxy = MAX(y, maxy);
			fillpoints[i].x = ROTX(x,y);
			fillpoints[i].y = ROTY(x,y);
		}

		/* set clipping to the last points of the arrowhead and
		 * the (enlarged) box surrounding it */
		*nclippts = 0;
		for (i = 0; i < arrow_shapes[indx].numpts -
				arrow_shapes[indx].startclip; ++i) {
			clippts[i].x =
				points[i + arrow_shapes[indx].startclip].x;
			clippts[i].y =
				points[i + arrow_shapes[indx].startclip].y;
		}

		/* now make the box around it at least as large as the line
		 * thickness */
		/* start with last x, upper y */
		if (halfthick > maxy) {
			x = arrow_shapes[indx].points[*npoints - 1].x * len;
			clippts[i].x = ROTX(x, clipthick);
			clippts[i].y = ROTY(x, clipthick);
			++i;
		}

		/* x tip (always == 0), same y (note different offset in ROTX/Y2
		   rotation) */
		/* add halfthick in case the cap style is Round or Projecting */
		clippts[i].x = ROTX2(clipthick, clipthick);
		clippts[i].y = ROTY2(clipthick, clipthick);
		++i;

		/* x tip, upper y (note different offset in ROTX/Y2
		   rotation) */
		/* add halfthick in case the cap style is Round or Projecting */
		clippts[i].x = ROTX2(clipthick, -clipthick);
		clippts[i].y = ROTY2(clipthick, -clipthick);
		++i;

		if (-halfthick < miny) {
			x = arrow_shapes[indx].points[
				arrow_shapes[indx].startclip].x * len;
			clippts[i].x = ROTX(x, -clipthick);
			clippts[i].y = ROTY(x, -clipthick);
			++i;
		}

		/* set the number of points in the clip or bounds */
		*nclippts = i;
	}
}

/* Computes a point on a line which is a chord to the arc specified by */
/* center (x1,y1) and endpoint (x2,y2), where the chord intersects the */
/* arc arrow->ht from the endpoint.                                    */
/* May give strange values if the arrow.ht is larger than about 1/4 of */
/* the circumference of a circle on which the arc lies.                */

void
compute_arcarrow_angle(double x1, double y1, double x2, double y2,
		int direction, F_arrow *arrow, int *x, int *y)
{
	double	 r, alpha, beta, dy, dx;
	double	 lpt,h;
	double	 thick;

	dy=y2-y1;
	dx=x2-x1;
	r=sqrt(dx*dx+dy*dy);
	h = (double) arrow->ht;
	/* lines are made a little thinner in set_linewidth */
	thick = arrow->thickness <= THICK_SCALE ?
		0.5 * arrow->thickness : arrow->thickness - THICK_SCALE;
	/* lpt is the amount the arrowhead extends beyond the end of the line */
	lpt = thick/2.0/(arrow->wid/h/2.0);
	/* add this to the length */
	h += lpt;

	/* secant would be too large or too small */
	if (h > 2.0*r || h < 0.01*r) {
		arc_tangent_int(x1,y1,x2,y2,direction,x,y);
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
