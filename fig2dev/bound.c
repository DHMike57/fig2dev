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

#include <stdio.h>
#include <math.h>
#include "pi.h"
#include "fig2dev.h"
#include "object.h"

#define		Ninety_deg		M_PI_2
#define		One_eighty_deg		M_PI
#define		Two_seventy_deg		(M_PI + M_PI_2)
#define		Three_sixty_deg		(M_PI + M_PI)
#define		half(z1 ,z2)		((z1+z2)/2.0)
#define		max(a, b)		(((a) > (b)) ? (a) : (b))
#define		min(a, b)		(((a) < (b)) ? (a) : (b))

arc_bound(arc, xmin, ymin, xmax, ymax)
F_arc	*arc;
int	*xmin, *ymin, *xmax, *ymax;
{
	double	alpha, beta;
	double	dx, dy, radius;
	int	bx, by, sx, sy;

	dx = arc->point[0].x - arc->center.x;
	dy = arc->center.y - arc->point[0].y;
	alpha = atan2(dy, dx);
	if (alpha < 0.0) alpha += Three_sixty_deg;
	/* compute_angle returns value between 0 to 2PI */
	
	radius = hypot(dx, dy);

	dx = arc->point[2].x - arc->center.x;
	dy = arc->center.y - arc->point[2].y;
	beta = atan2(dy, dx);
	if (beta < 0.0) beta += Three_sixty_deg;

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
		if (alpha <= Ninety_deg || Ninety_deg <= beta)
		    sy = (int)(arc->center.y - radius - 1.0);
		if (alpha <= One_eighty_deg || One_eighty_deg <= beta)
		    sx = (int)(arc->center.x - radius - 1.0);
		if (alpha <= Two_seventy_deg || Two_seventy_deg <= beta)
		    by = (int)(arc->center.y + radius + 1.0);
		}
	    else {
		if (0 <= beta && alpha <= 0)
		    bx = (int)(arc->center.x + radius + 1.0);
		if (Ninety_deg <= beta && alpha <= Ninety_deg)
		    sy = (int)(arc->center.y - radius - 1.0);
		if (One_eighty_deg <= beta && alpha <= One_eighty_deg)
		    sx = (int)(arc->center.x - radius - 1.0);
		if (Two_seventy_deg <= beta && alpha <= Two_seventy_deg)
		    by = (int)(arc->center.y + radius + 1.0);
		}
	    }
	else {	/* clockwise	*/
	    if (alpha > beta) {
		if (beta <= 0 && 0 <= alpha)
		    bx = (int)(arc->center.x + radius + 1.0);
		if (beta <= Ninety_deg && Ninety_deg <= alpha)
		    sy = (int)(arc->center.y - radius - 1.0);
		if (beta <= One_eighty_deg && One_eighty_deg <= alpha)
		    sx = (int)(arc->center.x - radius - 1.0);
		if (beta <= Two_seventy_deg && Two_seventy_deg <= alpha)
		    by = (int)(arc->center.y + radius + 1.0);
		}
	    else {
		if (0 <= alpha || beta <= 0)
		    bx = (int)(arc->center.x + radius + 1.0);
		if (Ninety_deg <= alpha || beta <= Ninety_deg)
		    sy = (int)(arc->center.y - radius - 1.0);
		if (One_eighty_deg <= alpha || beta <= One_eighty_deg)
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
	}

compound_bound(compound, xmin, ymin, xmax, ymax, include)
F_compound	*compound;
int		*xmin, *ymin, *xmax, *ymax;
int		include;
{
	F_arc		*a;
	F_ellipse	*e;
	F_compound	*c;
	F_spline	*s;
	F_line		*l;
	F_text		*t;
	int		bx, by, sx, sy, first = 1;
	int		llx, lly, urx, ury;
	int	        half_wd;

    while(compound != NULL) {
	for (a = compound->arcs; a != NULL; a = a->next) {
	    arc_bound(a, &sx, &sy, &bx, &by);
            half_wd = (a->thickness + 1) / 2;
	    if (first) {
		first = 0;
		llx = sx - half_wd; lly = sy - half_wd;
		urx = bx + half_wd; ury = by + half_wd;
		}
	    else {
		llx = min(llx, sx - half_wd); lly = min(lly, sy - half_wd);
		urx = max(urx, bx + half_wd); ury = max(ury, by + half_wd);
		}
	    }

	if (compound->compounds) {
	    compound_bound(compound->compounds, &sx, &sy, &bx, &by, include);
	    if (first) {
		first = 0;
		llx = sx; lly = sy;
		urx = bx; ury = by;
		}
	    else {
		llx = min(llx, sx); lly = min(lly, sy);
		urx = max(urx, bx); ury = max(ury, by);
		}
	    }

	for (e = compound->ellipses; e != NULL; e = e->next) {
	    ellipse_bound(e, &sx, &sy, &bx, &by);
	    if (first) {
		first = 0;
		llx = sx; lly = sy;
		urx = bx; ury = by;
		}
	    else {
		llx = min(llx, sx); lly = min(lly, sy);
		urx = max(urx, bx); ury = max(ury, by);
		}
	    }

	for (l = compound->lines; l != NULL; l = l->next) {
	    line_bound(l, &sx, &sy, &bx, &by);
            half_wd = ceil((double)(l->thickness+1) / sqrt(2.0)); 
            /* leave space for corners, better approach needs much more math! */
	    if (first) {
		first = 0;
		llx = sx - half_wd; lly = sy - half_wd;
		urx = bx + half_wd; ury = by + half_wd;
		}
	    else {
		llx = min(llx, sx - half_wd); lly = min(lly, sy - half_wd);
		urx = max(urx, bx + half_wd); ury = max(ury, by + half_wd);
		}
	    }

	for (s = compound->splines; s != NULL; s = s->next) {
	    spline_bound(s, &sx, &sy, &bx, &by);
            half_wd = (s->thickness+1) / 2;
	    if (first) {
		first = 0;
		llx = sx - half_wd; lly = sy - half_wd;
		urx = bx + half_wd; ury = by + half_wd;
		}
	    else {
		llx = min(llx, sx - half_wd); lly = min(lly, sy - half_wd);
		urx = max(urx, bx + half_wd); ury = max(ury, by + half_wd);
		}
	    }

	for (t = compound->texts; t != NULL; t = t->next) {
	    text_bound(t, &sx, &sy, &bx, &by, include);
	    if (first) {
		first = 0;
		llx = sx; lly = sy;
		urx = bx; ury = by;
		}
	    else {
		llx = min(llx, sx); lly = min(lly, sy);
		urx = max(urx, bx); ury = max(ury, by);
		}
	    }
        compound = compound->next;
    }

    *xmin = llx; *ymin = lly;
    *xmax = urx; *ymax = ury;
}

ellipse_bound(e, xmin, ymin, xmax, ymax)
F_ellipse	*e;
int		*xmin, *ymin, *xmax, *ymax;
{ 
	/* stolen from xfig-2.1.8 max2 from xfig == max here*/

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
	*xmin = *ymin =  100000;
	*xmax = *ymax = -100000;
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

line_bound(l, xmin, ymin, xmax, ymax)
F_line	*l;
int	*xmin, *ymin, *xmax, *ymax;
{
	points_bound(l->points, xmin, ymin, xmax, ymax);
	}

spline_bound(s, xmin, ymin, xmax, ymax)
F_spline	*s;
int		*xmin, *ymin, *xmax, *ymax;
{
	if (int_spline(s)) {
	    int_spline_bound(s, xmin, ymin, xmax, ymax);
	    }
	else {
	    normal_spline_bound(s, xmin, ymin, xmax, ymax);
	    }
	}

int_spline_bound(s, xmin, ymin, xmax, ymax)
F_spline	*s;
int		*xmin, *ymin, *xmax, *ymax;
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

normal_spline_bound(s, xmin, ymin, xmax, ymax)
F_spline	*s;
int		*xmin, *ymin, *xmax, *ymax;
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
	    }
	else {
	    *xmin = floor(min(sx, x2) );
	    *ymin = floor(min(sy, y2) );
	    *xmax = ceil (max(bx, x2) );
	    *ymax = ceil (max(by, y2) );
	    }
	}

double rot_x(x,y,angle) 
double x,y,angle;
{
    return(x*cos(-angle)-y*sin(-angle));
}

double rot_y(x,y,angle)
double x,y,angle;
{
 return(x*sin(-angle)+y*cos(-angle));
}


text_bound(t, xmin, ymin, xmax, ymax, include)
F_text	*t;
int	*xmin, *ymin, *xmax, *ymax;
int	include;
{
	double dx1, dx2, dx3, dx4, dy1, dy2, dy3, dy4;
	int descend;
	descend = (strchr(t->cstring,'g') || strchr(t->cstring,'j') ||
		  strchr(t->cstring,'p') || strchr(t->cstring,'q') ||
		  strchr(t->cstring,'y'));
	/* characters have some extent downside */
	if (t->type == T_CENTER_JUSTIFIED) {
	    dx1 = (t->length/1.95);     dy1 =  0.0;
	    dx2 = -(t->length/1.95);    dy2 =  0.0;
	    dx3 = (t->length/1.95);     dy3 = -t->height;
	    dx4 = -(t->length/1.95);    dy4 = -t->height;
	} else if (t->type == T_RIGHT_JUSTIFIED) {
	    dx1 = 0.0;                      dy1 =  0.0;
	    dx2 = -t->length*1.0256;        dy2 =  0.0;
	    dx3 = 0.0;                      dy3 = -t->height;
	    dx4 = -t->length*1.0256;        dy4 = -t->height;
	} else {
	    dx1 = (include ? t->length*1.0256 : 0); dy1 =  0;
	    dx2 = 0.0;                              dy2 =  0;
	    dx3 = (include ? t->length*1.0256 : 0); dy3 = -t->height;
	    dx4 = 0.0;                              dy4 = -t->height;
	}
    if (descend) {
	dy1 = 0.3*t->height;
	dy2 = 0.3*t->height;
	dy3 = -0.8*t->height;
	dy4 = -0.8*t->height;
    }
	
    *xmax= t->base_x +
           max( max( rot_x(dx1,dy1,t->angle), rot_x(dx2,dy2,t->angle) ), 
	        max( rot_x(dx3,dy3,t->angle), rot_x(dx4,dy4,t->angle) ) );
    *ymax= t->base_y + 
           max( max( rot_y(dx1,dy1,t->angle), rot_y(dx2,dy2,t->angle) ), 
	        max( rot_y(dx3,dy3,t->angle), rot_y(dx4,dy4,t->angle) ) );

    *xmin= t->base_x + 
           min( min( rot_x(dx1,dy1,t->angle), rot_x(dx2,dy2,t->angle) ), 
	        min( rot_x(dx3,dy3,t->angle), rot_x(dx4,dy4,t->angle) ) );
    *ymin= t->base_y + 
           min( min( rot_y(dx1,dy1,t->angle), rot_y(dx2,dy2,t->angle) ), 
	        min( rot_y(dx3,dy3,t->angle), rot_y(dx4,dy4,t->angle) ) );
	}

points_bound(points, xmin, ymin, xmax, ymax)
F_point	*points;
int	*xmin, *ymin, *xmax, *ymax;
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

control_points_bound(cps, xmin, ymin, xmax, ymax)
F_control	*cps;
int		*xmin, *ymin, *xmax, *ymax;
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
