/*
 * TransFig: Facility for Translating Fig code
 *
 *  Copyright (c) 1993 Anthony Starks (ajs@merck.com)
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
 *  fig2MF -- convert fig to mfpic METAFONT code
 *
 *  Copyright (c) 1993 Anthony Starks (ajs@merck.com)
 *
 *  Version 0.00 -- 	Incorporate Tobin's small suggestions
 *  Version 0.01 -- 	Change scaling to inches,
 *			default [x-y]scale is 1/8 inch
 *			slight format changes in cinit()
 *  Version 0.02 --	Fixed pen switching bug
 *  Version 0.03 --	Support new arcthree mode
 *  Version 0.04 --	Token support for text
 *  Version 0.05 --	Integrated into fig2dev - B.V.Smith 11/28/94
 */

#include <stdio.h>
#include "fig2dev.h"
#include "object.h"

#define dofill(obj)	1.2-((double)obj->fill_style/(double)BLACK_FILL)
#define dopen(x)	((x-1)*PEN_INCR)+DEF_PEN
#define VERSION		0.05
#define DEF_PEN		0.5
#define PEN_INCR	0.10

typedef struct {
	char *keyword;
	double *value;
} Options;

static double ppi;
static double code = 32.0;
static double mfpen = 0.5;
static double xscale = 0.125;
static double yscale = 0.125;
static double xl = 0.0;
static double yl = 8.0;
static double xu = 0.0;
static double yu = 8.0;
static double maxy = 8.0;

void
genmf_start(objects)
F_compound	*objects;
{
	int	curchar;

	ppi = objects->nwcorner.x/mag;
	curchar = (int)code;

	fprintf(tfp,"%%\n%% fig2dev -L mf version %.2lf --- Preamble\n%%\n",
		VERSION);
	fprintf(tfp,"mag:=%g/1000; input graphbase.mf; code:=%g;\n",
		mag, code);
	fprintf(tfp,"mfpicenv;\ninterim hdwdr:=1; interim hdten:=1;\n");
	fprintf(tfp,"interim penwd:=%.2lfpt;\npickup pencircle scaled penwd;\n", mfpen);

	fprintf(tfp,"%%\n%% %s (char %d)\n%%\n",
		(name? name: ((from) ? from : "stdin")), ++curchar);
	fprintf(tfp,"xscale:=%.3lf; yscale:=%.3lf; bounds(%.3lf,%.3lf,%.3lf,%.3lf);\n",
		xscale, yscale, xl, xu, yl, yu);
	fprintf(tfp,"beginchar(incr code,xscale*(xpos-xneg)*in#,yscale*(ypos-yneg)*in#,0);\n");
	fprintf(tfp,"  setztr;\n");
	fprintf(tfp,"  pickup pencircle scaled %.2lfpt;\n", mfpen);
}


void
genmf_end()
{
	fprintf(tfp,"endmfpicenv;\nend.\n");
}

void
genmf_option(opt, optarg)
char opt;
char *optarg;
{
    switch (opt) {
	case 'C':
	    code = atof(optarg);
	    break;
	case 'n':
	    name = optarg;
	    break;
	case 'p':
	    mfpen = atof(optarg);
	    break;
	case 't':
	    maxy = atof(optarg);
	    break;
	case 'x':
	    xl = atof(optarg);
	    break;
	case 'y':
	    yl = atof(optarg);
	    break;
	case 'X':
	    xu = atof(optarg);
	    break;
	case 'Y':
	    yu = atof(optarg);
	    break;

    }
}

void
genmf_line(l)
F_line *l;
{
	F_point	*p;
	if (l->thickness > 1)
		fprintf(tfp,"  pickup pencircle scaled %.2lfpt;\n",
			dopen(l->thickness));
	if (l->fill_style == BLACK_FILL)
		fprintf(tfp,"  cycleshade(0, false,\n");
	else if (l->fill_style < BLACK_FILL && l->fill_style > 0)
		fprintf(tfp,"  cycleshade(%lfpt, false,\n", dofill(l));
	else
		fprintf(tfp,"  curve(false, false,\n");
	p = l->points;
	fprintf(tfp,"       (%lf, %lf)", p->x/ppi, maxy-(p->y/ppi));
	p = p->next;
	for ( ; p != NULL; p=p->next) {
	    fprintf(tfp,",\n       (%lf, %lf)", p->x/ppi, maxy-(p->y/ppi));
	}
	if (l->thickness > 1)
		fprintf(tfp,"  pickup pencircle scaled %.2lfpt;\n",
			mfpen);

	cfin();
	return;
}


void
genmf_spline(s)
F_spline *s;
{
	F_point	*p;
	if (s->thickness > 1)
		fprintf(tfp,"  pickup pencircle scaled %.2lfpt;\n",
			dopen(s->thickness));
	if (s->fill_style == BLACK_FILL)
		fprintf(tfp,"  cycleshade(0, true,\n");
	else if (s->fill_style < BLACK_FILL && s->fill_style > 0)
		fprintf(tfp,"  cycleshade(%lfpt, true,\n", dofill(s));
	else
		fprintf(tfp,"  curve(true, false,\n");
	p = s->points;
	fprintf(tfp,"       (%lf, %lf)", p->x/ppi, maxy-(p->y/ppi));
	p = p->next;
	for ( ; p != NULL; p=p->next) {
	    fprintf(tfp,",\n       (%lf, %lf)", p->x/ppi, maxy-(p->y/ppi));
	}
	if (s->thickness > 1)
		fprintf(tfp,"  pickup pencircle scaled %.2lfpt;\n", mfpen);
	cfin();
	return;
}


void
genmf_ellipse(e)
F_ellipse *e;
{
	if (e->thickness > 1)
		fprintf(tfp,"  pickup pencircle scaled %.2lfpt;\n",
			dopen(e->thickness));

	if (e->type == 3 || e->type == 4)
	{
		if (e->fill_style == BLACK_FILL)
			fprintf(tfp,"  circshade(0, ");
		else if (e->fill_style < BLACK_FILL && e->fill_style > 0)
			fprintf(tfp,"  circshade(%lfpt, ", dofill(e));
		else
			fprintf(tfp,"  circle(");
		fprintf(tfp,"(%lf,%lf),%lf);\n",
			e->center.x/ppi, maxy-(e->center.y/ppi), e->radiuses.x/ppi);
	}
	else if (e->type == 1 || e->type == 2)
	{
		if (e->fill_style == BLACK_FILL)
			fprintf(tfp,"  ellshade(0, ");
		else if (e->fill_style < BLACK_FILL && e->fill_style > 0)
			fprintf(tfp,"  ellshade(%lfpt, ", dofill(e));
		else
			fprintf(tfp,"  ellipse(");
		fprintf(tfp,"(%lf,%lf),%lf,%lf,0);\n",
			e->center.x/ppi, maxy-(e->center.y/ppi), 
			e->radiuses.x/ppi, e->radiuses.y/ppi);
	}
	if (e->thickness > 1)
		fprintf(tfp,"  pickup pencircle scaled %.2lfpt;\n", mfpen);

}

void
genmf_arc(a)
F_arc *a;
{

	if (a->thickness > 1)
		fprintf(tfp,"  pickup pencircle scaled %.2lfpt;\n",
			dopen(a->thickness));

	fprintf(tfp,"  arcthree((%lf,%lf), (%lf,%lf), (%lf,%lf));\n",
		a->point[0].x/ppi, maxy-(a->point[0].y/ppi),
		a->point[1].x/ppi, maxy-(a->point[1].y/ppi),
		a->point[2].x/ppi, maxy-(a->point[2].y/ppi));

	if (a->thickness > 1)
		fprintf(tfp,"  pickup pencircle scaled %.2lfpt;\n",
			mfpen);

	cfin();
	return;

}

void
genmf_text(t)
F_text *t;
{
	fprintf(tfp,"%% label((%lf,%lf),%s)\n", t->base_x/ppi,
		maxy-(t->base_y/ppi), t->cstring);
	cfin();
	return;
}

cfin()
{
	fprintf(tfp,"endchar;\n");
}

struct driver dev_mf = {
     	genmf_option,
	genmf_start,
	genmf_arc,
	genmf_ellipse,
	genmf_line,
	genmf_spline,
	genmf_text,
	genmf_end,
	INCLUDE_TEXT
};
