/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1985 Supoj Sutantavibul
 * Copyright (c) 1991 Micah Beck
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
 */

#include "fig2dev.h"
#include "figure.h"
#include "object.h"

#define		Ninety_deg		M_PI_2
#define		One_eighty_deg		M_PI
#define		Two_seventy_deg		(M_PI + M_PI_2)
#define		Three_sixty_deg		(M_PI + M_PI)
#define		half(z1 ,z2)		((z1+z2)/2.0)
#define		max(a, b)		(((a) > (b)) ? (a) : (b))
#define		min(a, b)		(((a) < (b)) ? (a) : (b))

static double	compute_angle();

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
	
	radius = sqrt(dx*dx + dy*dy);

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

	/* now add in the arrow (if any) boundaries */
	arrow_bound(O_ARC, (F_line *)arc, xmin, ymin, xmax, ymax);
	}

compound_bound(compound, xmin, ymin, xmax, ymax, include)
    F_compound	*compound;
    int		*xmin, *ymin, *xmax, *ymax;
    int		 include;
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
	    /* pictures have no line thickness */
	    if (l->type == T_PIC_BOX)
		half_wd = 0;
	    else
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

line_bound(l, xmin, ymin, xmax, ymax)
F_line	*l;
int	*xmin, *ymin, *xmax, *ymax;
{
	points_bound(l->points, xmin, ymin, xmax, ymax);
	/* now add in the arrow (if any) boundaries but
	   only if the line has two or more points */
	if (l->points->next)
	    arrow_bound(O_POLYLINE, l, xmin, ymin, xmax, ymax);
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
	/* now do any arrows */
	arrow_bound(O_SPLINE, s, xmin, ymin, xmax, ymax);
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
    double	x,y,angle;
{
    return(x*cos(-angle)-y*sin(-angle));
}

double rot_y(x,y,angle)
    double	x,y,angle;
{
    return(x*sin(-angle)+y*cos(-angle));
}


text_bound(t, xmin, ymin, xmax, ymax, inc_text)
    F_text	*t;
    int		*xmin, *ymin, *xmax, *ymax;
    int		 inc_text;
{
    double	 dx1, dx2, dx3, dx4, dy1, dy2, dy3, dy4;
    int		 descend;

    Boolean	 include;
    
    include = (inc_text &&
			(t->flags & SPECIAL_TEXT)==0 || strchr(t->cstring,'\\')==0);
    /* look for descenders in string (this is a kludge - next version 
       of xfig should include ascent/descent in text structure */
    descend = (strchr(t->cstring,'g') || strchr(t->cstring,'j') ||
		  strchr(t->cstring,'p') || strchr(t->cstring,'q') ||
		  strchr(t->cstring,'y') || strchr(t->cstring,'$') ||
		  strchr(t->cstring,'(') || strchr(t->cstring,')') ||
		  strchr(t->cstring,'{') || strchr(t->cstring,'}') ||
		  strchr(t->cstring,',') || strchr(t->cstring,';'));
    /* characters have some extent downside */
    if (t->type == T_CENTER_JUSTIFIED) {
	dx1 = (include?  (t->length/1.95) : 0.0);	dy1 =  0.0;
	dx2 = (include? -(t->length/1.95) : 0.0);	dy2 =  0.0;
	dx3 = (include?  (t->length/1.95) : 0.0);	dy3 = -t->height;
	dx4 = (include? -(t->length/1.95) : 0.0);	dy4 = -t->height;
    } else if (t->type == T_RIGHT_JUSTIFIED) {
	dx1 = 0.0;					dy1 =  0.0;
	dx2 = (include? -t->length*1.0256 : 0.0);	dy2 =  0.0;
	dx3 = 0.0;					dy3 = -t->height;
	dx4 = (include? -t->length*1.0256 : 0.0);	dy4 = -t->height;
    } else {
	dx1 = (include ? t->length*1.0256 : 0.0);	dy1 =  0.0;
	dx2 = 0.0;					dy2 =  0.0;
	dx3 = (include ? t->length*1.0256 : 0.0);	dy3 = -t->height;
	dx4 = 0.0;					dy4 = -t->height;
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
    int		*xmin, *ymin, *xmax, *ymax;
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

/* extend xmin, ymin xmax, ymax by the arrow boundaries of obj (if any) */

arrow_bound(objtype, obj, xmin, ymin, xmax, ymax)
    int		    objtype;
    F_line	   *obj;
    int		   *xmin, *ymin, *xmax, *ymax;
{
    int		    fxmin, fymin, fxmax, fymax;
    int		    bxmin, bymin, bxmax, bymax;
    F_point	   *p, *q;
    F_arc	   *a;
    int		    p1x, p1y, p2x, p2y;
    int		    dum;
    int		    npts, i;
    Point	    arrowpts[50];

    if (obj->for_arrow) {
	if (objtype == O_ARC) {
	    a = (F_arc *) obj;
	    compute_arcarrow_angle(a->center.x, a->center.y,
			(double)a->point[2].x, (double)a->point[2].y,
			a->direction, a->for_arrow, &p1x, &p1y);
	    p2x = a->point[2].x;	/* forward tip */
	    p2y = a->point[2].y;
	} else {
	    /* this doesn't work very well for a spline with few points 
		and lots of curvature */
	    /* locate last point (forward tip) and next-to-last point */
	    for (p = obj->points; p->next; p = p->next)
		q = p;
	    p1x = q->x;
	    p1y = q->y;
	    p2x = p->x;
	    p2y = p->y;
	}
	calc_arrow(p1x, p1y, p2x, p2y, &dum, &dum, &dum, &dum,
			obj->for_arrow, arrowpts, &npts, &dum);
	fxmin=fymin=10000000;
	fxmax=fymax=-10000000;
	for (i=0; i<npts; i++) {
	    fxmin = min2(fxmin, arrowpts[i].x);
	    fymin = min2(fymin, arrowpts[i].y);
	    fxmax = max2(fxmax, arrowpts[i].x);
	    fymax = max2(fymax, arrowpts[i].y);
	}
	*xmin = min2(*xmin, fxmin);
	*xmax = max2(*xmax, fxmax);
	*ymin = min2(*ymin, fymin);
	*ymax = max2(*ymax, fymax);
    }
    if (obj->back_arrow) {
	if (objtype == O_ARC) {
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
	calc_arrow(p1x, p1y, p2x, p2y, &dum, &dum, &dum, &dum,
			obj->back_arrow, arrowpts, &npts, &dum);
	bxmin=bymin=10000000;
	bxmax=bymax=-10000000;
	for (i=0; i<npts; i++) {
	    bxmin = min2(bxmin, arrowpts[i].x);
	    bymin = min2(bymin, arrowpts[i].y);
	    bxmax = max2(bxmax, arrowpts[i].x);
	    bymax = max2(bymax, arrowpts[i].y);
	}
	*xmin = min2(*xmin, bxmin);
	*xmax = max2(*xmax, bxmax);
	*ymin = min2(*ymin, bymin);
	*ymax = max2(*ymax, bymax);
    }
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
    *nboundpts = 0;
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
    miny =  10000000.0;
    maxy = -10000000.0;
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

/* Computes a point on a line which is a chord to the arc specified by */
/* center (x1,y1) and endpoint (x2,y2), where the chord intersects the */
/* arc arrow->ht from the endpoint.                                    */
/* May give strange values if the arrow.ht is larger than about 1/4 of */
/* the circumference of a circle on which the arc lies.                */

compute_arcarrow_angle(x1, y1, x2, y2, direction, arrow, x, y)
    double	 x1, y1;
    double	 x2, y2;
    int		 direction;
    F_arrow	*arrow;
    int		*x, *y;
{
    double	 r, alpha, beta, dy, dx;
    double	 lpt,h;
    double	 thick;

    dy=y2-y1;
    dx=x2-x1;
    r=sqrt(dx*dx+dy*dy);
    h = (double) arrow->ht;
    /* lines are made a little thinner in set_linewidth */
    thick = (arrow->thickness <= THICK_SCALE) ? 	
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

