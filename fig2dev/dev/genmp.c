/*
 * TransFig: Facility for Translating Fig code
 *
 * Ported from fig2MP by Klaus Guntermann (guntermann@iti.informatik.tu-darmstadt.de)
 * Original fig2MP Copyright 1995 Dane Dwyer (dwyer@geisel.csl.uiuc.edu)
 * 
 *
 * Any party obtaining a copy of these files is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and
 * documentation files (the "Software"), including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons who receive
 * copies from any such party to do so, with the only requirement being
 * that this copyright notice and the one following remain intact.
 *
 */

/*
 *  fig2MP -- convert fig to METAPOST code
 *
 *  Copyright 1995 Dane Dwyer (dwyer@geisel.csl.uiuc.edu)
 *  Written while a student at the University of Illinois at Urbana-Champaign
 *
 *  Permission is granted to freely distribute this program provided
 *          this copyright notice is included.
 *
 *  Version 0.00 --  Initial attempt  (8/20/95)
 *  Version 0.01 --  Added user defined colors  (8/27/95)
 *                   Added filled and unfilled arrowheads  (8/27/95)
 *  Version 0.02 --  Changed default mapping of ps fonts to match teTeX
 *                    (standard?)  (9/16/96)
 *
 *  Development for new extensions at TU Darmstadt, Germany starting 1999
 *
 *  Version 0.03 --  Allow to "build" pictures incrementally.
 *                   To achieve this we split the complete figure into
 *                   layers in separate mp-figures. The complete figure
 *                   will be seen when overlapping all layers.
 *                   A layer is combined from adjacent depths in xfig.
 *                   This makes it possible to overlap items also when
 *                   splitting into layers.
 *                   The layered version creates files with extension
 *                   ".mmp" (multi-level MetaPost). These can be processed
 *                   with normal MetaPost, but you have to provide the
 *                   extension of the filename in the call.
 */

/*
 *  Limitations:
 *         No fill patterns (just shades) -- Not supported by MetaPost
 *         No embedded images -- Not supported by MetaPost
 *         Only flatback arrows
 *
 *  Assumptions:
 *         11" high paper (can be easily changed below)
 *         xfig coordinates are 1200 pixels per inch
 *         xfig fonts scaled by 72/80
 *         Output is for MetaPost version 0.63
 */

#include "fig2dev.h"
#include "object.h"

/*
 *  Definitions
 */
#define GENMP_VERSION	0.03
#define PaperHeight	11.0	/* inches */
#define BPperINCH	72.0
#define MaxyBP		BPperINCH*PaperHeight
#define PIXperINCH	1200.0
#define FIGSperINCH	80.0

/*
 *  Special functions
 */
char	*genmp_fillcolor(int, int);
char	*genmp_pencolor(int);
void	genmp_writefontinfo(double, char *, char *);
void	genmp_arrowstats(F_arrow *);
void	do_split(int); /* new procedure to split different depths' objects */
#define	getfont(x,y)	((x & 0x4)?xfigpsfonts[y]:xfiglatexfonts[y])
#define	fig2bp(x)	((double)x * BPperINCH/PIXperINCH)

#define	rad2deg(x)	((double)x * 180.0/3.141592654)
#define	y_off(x)	(MaxyBP-(double)(x))   /* reverse y coordinate */

/* Next two used to generate random font names - AA, AB, AC, ... ZZ */
#define	char1()		(((int)(c1++/26) % 26)+'A')
#define	char2()		((c2++ % 26)+'A')

/*
 *  Global variables
 */
char c1=0,c2=0;

int split = False; /* no splitting is default */

/*
 *  Static variables for variant mmp:
 *   fig_number has the "current" figure number which has been created.
 *   last_depth remembers the last level number processed
 *         (we need a sufficiently large initial value)
 */
static int fig_number=0;
static int last_depth=1001;
/*
 * default xfig postscript fonts given TeX equivalent names
 */
#define FONTNAMESIZE	8
char xfigpsfonts[35][FONTNAMESIZE] = {
        "ptmr",         /* Times-Roman */
        "ptmri",        /* Times-Italic */
        "ptmb",         /* Times-Bold */
        "ptmbi",        /* Times-BoldItalic */
        "pagk",         /* AvantGarde-Book */
        "pagko",        /* AvantGarde-BookOblique */
        "pagd",         /* AvantGarde-Demi */
        "pagdo",        /* AvantGarde-DemiOblique */
        "pbkl",         /* Bookman-Light */
        "pbkli",        /* Bookman-LightItalic */
        "pbkd",         /* Bookman-Demi */
        "pbkdi",        /* Bookman-DemiItalic */
        "pcrr",         /* Courier */
        "pcrro",        /* Courier-Oblique */
        "pcrb",         /* Courier-Bold */
        "pcrbo",        /* Courier-BoldOblique */
        "phvr",         /* Helvetica */
        "phvro",        /* Helvetica-Oblique */
        "phvb",         /* Helvetica-Bold */
        "phvbo",        /* Helvetica-BoldOblique */
        "phvrrn",       /* Helvetica-Narrow */
        "phvron",       /* Helvetica-Narrow-Oblique */
        "phvbrn",       /* Helvetica-Narrow-Bold */
        "phvbon",       /* Helvetica-Narrow-BoldOblique */
        "pncr",         /* NewCenturySchlbk-Roman */
        "pncri",        /* NewCenturySchlbk-Italic */
        "pncb",         /* NewCenturySchlbk-Bold */
        "pncbi",        /* NewCenturySchlbk-BoldItalic */
        "pplr",         /* Palatino-Roman */
        "pplri",        /* Palatino-Italic */
        "pplb",         /* Palatino-Bold */
        "pplbi",        /* Palatino-BoldItalic */
        "psyr",         /* Symbol */
        "pzcmi",        /* ZapfChancery-MediumItalic */
        "pzdr"          /* ZapfDingbats */
};

/*
 * default xfig latex fonts given TeX equivalent names
 */
char xfiglatexfonts[6][FONTNAMESIZE] = {
	"cmr10",         /* DEFAULT = Computer Modern Roman */
	"cmr10",         /* Computer Modern Roman */
	"cmbx10",        /* Computer Modern Roman Bold */
	"cmit10",        /* Computer Modern Roman Italic */
	"cmss10",        /* Computer Modern Sans Serif */
	"cmtt10",        /* Computer Modern Typewriter */
};

/*
 * default xfig colors in postscript RGB notation
 */
char xfigcolors[32][17] = {
	"(0.00,0.00,0.00)",		/* black */
	"(0.00,0.00,1.00)",		/* blue */
	"(0.00,1.00,0.00)",		/* green */
	"(0.00,1.00,1.00)",		/* cyan */
	"(1.00,0.00,0.00)",		/* red */
	"(1.00,0.00,1.00)",		/* magenta */
	"(1.00,1.00,0.00)",		/* yellow */
	"(1.00,1.00,1.00)",		/* white */
	"(0.00,0.00,0.56)",		/* blue1 */
	"(0.00,0.00,0.69)",		/* blue2 */
	"(0.00,0.00,0.82)",		/* blue3 */
	"(0.53,0.81,1.00)",		/* blue4 */
	"(0.00,0.56,0.00)",		/* green1 */
	"(0.00,0.69,0.00)",		/* green2 */
	"(0.00,0.82,0.00)",		/* green3 */
	"(0.00,0.56,0.56)",		/* cyan1 */
	"(0.00,0.69,0.69)",		/* cyan2 */
	"(0.00,0.82,0.82)",		/* cyan3 */
	"(0.56,0.00,0.00)",		/* red1 */
	"(0.69,0.00,0.00)",		/* red2 */
	"(0.82,0.00,0.00)",		/* red3 */
	"(0.56,0.00,0.56)",		/* magenta1 */
	"(0.69,0.00,0.69)",		/* magenta2 */
	"(0.82,0.00,0.82)",		/* magenta3 */
	"(0.50,0.19,0.00)",		/* brown1 */
	"(0.63,0.25,0.00)",		/* brown2 */
	"(0.75,0.38,0.00)",		/* brown3 */
	"(1.00,0.50,0.50)",		/* pink1 */
	"(1.00,0.63,0.63)",		/* pink2 */
	"(1.00,0.75,0.75)",		/* pink3 */
	"(1.00,0.88,0.88)",		/* pink4 */
	"(1.00,0.84,0.00)"		/* gold */
};

void
genmp_start(objects)
F_compound	*objects;
{
	fprintf(tfp,"%%\n%% fig2dev (version %s.%s) -L (m)mp version %.2lf --- Preamble\n%%\n",
	   VERSION, PATCHLEVEL, GENMP_VERSION);
	fprintf(tfp,"\n");

	/* print any whole-figure comments prefixed with "%" */
	if (objects->comments) {
	    fprintf(tfp,"%%\n");
	    print_comments("% ",objects->comments, "");
	    fprintf(tfp,"%%\n");
	}

	fprintf(tfp,"\n");
	fprintf(tfp,"%% Make arrowheads mitered by default\n");
	fprintf(tfp,"%% NOTE: subject to change (edited from plain.mp)\n");
	fprintf(tfp,"    def forwarr(text t) expr p =\n");
	fprintf(tfp,"      _apth:=p;_finarrf(t)\n");
	fprintf(tfp,"    enddef;\n");
	fprintf(tfp,"    def backarr(text t) expr p =\n");
	fprintf(tfp,"      _apth:=p;_finarrb(t)\n");
	fprintf(tfp,"    enddef;\n");
	fprintf(tfp,"    def _finarrf(text s) text t =\n");
	fprintf(tfp,"      if (s=0):fill arrowhead _apth  t withcolor white\n");
	fprintf(tfp,"      else: fill arrowhead _apth  t fi;\n");
	fprintf(tfp,"      linejoin:=0;\n");
	fprintf(tfp,"      draw arrowhead _apth  t\n");
	fprintf(tfp,"    enddef;\n");
	fprintf(tfp,"    def _finarrb(text s) text t =\n");
	fprintf(tfp,"      if (s=0):fill arrowhead reverse _apth  t withcolor white\n");
	fprintf(tfp,"      else: fill arrowhead reverse _apth  t fi;\n");
	fprintf(tfp,"      linejoin:=0;\n");
	fprintf(tfp,"      draw arrowhead reverse _apth  t\n");
	fprintf(tfp,"    enddef;\n");
	fprintf(tfp,"\n\n");

	if (split) {
	      /*
	       * We need a bounding box around all objects in all
	       * partial figures. Otherwise overlapping them will not
	       * work properly. We create this bounding area here once
	       * and for all. It will be included in each figure.
	       */
	      fprintf(tfp,"path allbounds;\n");
	      fprintf(tfp,"allbounds = (%.2lf,%.2lf)--(%.2lf,%.2lf)",
		      fig2bp(llx),y_off(fig2bp(lly)),fig2bp(urx),
		      y_off(fig2bp(lly)));
	      fprintf(tfp,"--(%.2lf,%.2lf)--(%.2lf,%.2lf)--cycle;\n",
		      fig2bp(urx),y_off(fig2bp(ury)),
		      fig2bp(llx),y_off(fig2bp(ury)));
	} else {
	   fprintf(tfp,"%% Now draw the figure\n");
	   fprintf(tfp,"beginfig(0)\n");
	}
	fprintf(tfp,"%% Some reasonable defaults\n");
	fprintf(tfp,"  ahlength:=7;\n");
	fprintf(tfp,"  ahangle:=30;\n");
	fprintf(tfp,"  labeloffset:=0;\n");
	/* For our overlapping figures we must keep the (invisible) bounding
	 * box as the delimiting area. Thus we must not have MetaPost
	 * compute the "true" bounds.
	 */
	fprintf(tfp,"  truecorners:=%d;\n",split==0);
	fprintf(tfp,"  bboxmargin:=0;\n");
        }


int
genmp_end()
{
        if (split) {
            /* We must add the bounds for the last figure */
	    fprintf(tfp,"setbounds currentpicture to allbounds;\n");
        }
        /* Close the (last) figure and terminate MetaPost program */
	fprintf(tfp,"endfig;\nend\n");
	return(0);
}

void
genmp_option(opt, optarg)
char opt, *optarg;
{
      if (strcasecmp(optarg,"mmp") == 0) split = True;
}

void
genmp_line(l)
F_line *l;
{
	F_point	*p;

	do_split(l->depth);

	/* print any comments prefixed with "%" */
	print_comments("% ",l->comments, "");

	fprintf(tfp,"%% Begin polyline object\n");
	switch( l->type) {
	   case 1:            /* Polyline */
	   case 2:            /* Box */
	   case 3:            /* Polygon */
	      fprintf(tfp,"  linecap:=%d;\n",l->cap_style);
	      fprintf(tfp,"  linejoin:=%d;\n",l->join_style);
	      fprintf(tfp,"  pickup pencircle scaled %.2lf;\n",fig2bp(l->thickness));
	      fprintf(tfp,"  path p;\n");
	      p = l->points;
	      fprintf(tfp,"  p = (%.2lf, %.2lf)", fig2bp(p->x),y_off(fig2bp(p->y)));
	      p = p->next;
	      for ( ; p != NULL; p=p->next) {
	         fprintf(tfp,"\n    --(%.2lf, %.2lf)", fig2bp(p->x),
	             y_off(fig2bp(p->y)));
	      }
	      if (l->type != 1)
	         fprintf(tfp,"--cycle;\n");
	      else
	         fprintf(tfp,";\n");
	      if (l->fill_style != -1) {   /* Filled? */
	         fprintf(tfp,"  path f;\n");
	         fprintf(tfp,"  f = p--cycle;\n");
	         fprintf(tfp,"  fill f %s;\n",
	            genmp_fillcolor(l->fill_color,l->fill_style));
	      }
         if (l->thickness != 0) {     /* invisible pen? */
	         fprintf(tfp,"  draw p ");
	         fprintf(tfp,"withcolor %s",genmp_pencolor(l->pen_color));
	         if (l->style == 1) {     /* dashed */
	            fprintf(tfp," dashed evenly scaled %.2lf;\n",l->style_val/3.3);
	         } else if (l->style == 2) {     /* dotted */
	            fprintf(tfp," dashed withdots scaled %.2lf;\n",l->style_val/4.75);
	         } else           /* plain */
	            fprintf(tfp,";\n");
	         if ((l->for_arrow != NULL) || (l->back_arrow != NULL)) {
	            if (l->for_arrow != NULL) {
	               genmp_arrowstats(l->for_arrow);
	               fprintf(tfp,"  forwarr(%d) p ",(l->for_arrow)->style);
	               fprintf(tfp,"withcolor %s;\n",genmp_pencolor(l->pen_color));
	            }
	            if (l->back_arrow != NULL) {
	               genmp_arrowstats(l->back_arrow);
	               fprintf(tfp,"  backarr(%d) p ",(l->back_arrow)->style);
	               fprintf(tfp,"withcolor %s;\n",genmp_pencolor(l->pen_color));
	            }
	         }
	      }
	      break;
	   case 4:            /* arc box */
	      fprintf(tfp,"  linecap:=%d;\n",l->cap_style);
	      fprintf(tfp,"  linejoin:=1;\n");   /* rounded necessary for arcbox */
	      fprintf(tfp,"  pickup pencircle scaled %.2lf;\n",fig2bp(l->thickness));
	      fprintf(tfp,"  path p,pb,sw,nw,ne,se;\n");
	      fprintf(tfp,"  pair ll,ul,ur,lr;\n");
	      p = l->points;
	      fprintf(tfp,"  p = (%.2lf,%.2lf)--",fig2bp(p->x),y_off(fig2bp(p->y)));
	      p = (p->next)->next;
         fprintf(tfp,"(%.2lf,%.2lf);\n",fig2bp(p->x),y_off(fig2bp(p->y)));
	      fprintf(tfp,"  ur = urcorner p; ll = llcorner p;\n");
 	      fprintf(tfp,"  ul = ulcorner p; lr = lrcorner p;\n");
	      fprintf(tfp,"  sw = fullcircle scaled %.2lf shifted (ll+\
(%.2lf,%.2lf));\n",fig2bp(l->radius*2),fig2bp(l->radius),fig2bp(l->radius));
	      fprintf(tfp,"  nw = fullcircle scaled %.2lf shifted (ul+\
(%.2lf,%.2lf));\n",fig2bp(l->radius*2),fig2bp(l->radius),-fig2bp(l->radius));
	      fprintf(tfp,"  ne = fullcircle rotated 180 scaled %.2lf shifted (ur+\
(%.2lf,%.2lf));\n",fig2bp(l->radius*2),-fig2bp(l->radius),-fig2bp(l->radius));
	      fprintf(tfp,"  se = fullcircle rotated 180 scaled %.2lf shifted (lr+\
(%.2lf,%.2lf));\n",fig2bp(l->radius*2),-fig2bp(l->radius),fig2bp(l->radius));
	      fprintf(tfp,"  pb = buildcycle(sw,ll--ul,nw,ul--ur,ne,ur--lr,se,\
lr--ll);\n");
	      if (l->fill_style != -1)
 	         fprintf(tfp,"  fill pb %s;\n",
	            genmp_fillcolor(l->fill_color,l->fill_style));
	      if (l->thickness != 0) {      /* invisible pen? */
	         fprintf(tfp,"  draw pb withcolor %s",
	            genmp_pencolor(l->pen_color));
	         if (l->style == 1) {     /* dashed */
	            fprintf(tfp," dashed evenly scaled %.2lf;\n",l->style_val/3.3);
	         } else if (l->style == 2) {     /* dotted */
	            fprintf(tfp," dashed withdots scaled %.2lf;\n",
	            l->style_val/4.75);
	         } else           /* plain */
	            fprintf(tfp,";\n");
	      }
	      break;
	   case 5:            /* picture object */
	      fprintf(tfp,"  show \"Picture objects are not supported!\"\n");
	      break;
	   default:
	      fprintf(tfp,"  show \"This Polyline object is not supported!\"\n");
	}
	fprintf(tfp,"%% End polyline object\n");
	return;
}

void
genmp_spline(s)
F_spline *s;
{
	F_point	*p;
	F_control *c;
	int i,j;

	do_split(s->depth);

	/* print any comments prefixed with "%" */
	print_comments("% ",s->comments, "");

	fprintf(tfp,"%% Begin spline object\n");
	switch (s->type) {
	   case 0:        /* control point spline (open) */
	   case 1:        /* control point spline (closed) */
	      fprintf(tfp,"  linecap:=%d;\n",s->cap_style);
	      fprintf(tfp,"  pair p[],c[];\n");
	      fprintf(tfp,"  path s;\n");
	      fprintf(tfp,"  pickup pencircle scaled %.2lf;\n",fig2bp(s->thickness));
/* locate "curve points" halfway between given poinsts */
	      p = s->points; i=0;
	      fprintf(tfp,"  p%d = .5[(%.2lf,%.2lf),", i++, fig2bp(p->x),
	         y_off(fig2bp(p->y)));
	      p = p->next;
	      fprintf(tfp,"(%.2lf,%.2lf)];\n",fig2bp(p->x),y_off(fig2bp(p->y)));
	      for ( ; p->next != NULL; ) {    /* at least 3 points */
	         fprintf(tfp,"  p%d = .5[(%.2lf,%.2lf),",i++, fig2bp(p->x),
	            y_off(fig2bp(p->y)));
	         p = p->next;
	         fprintf(tfp,"(%.2lf,%.2lf)];\n",fig2bp(p->x),y_off(fig2bp(p->y)));
	      }
/* locate "control points" 2/3 of way between curve points and given points */
	      p = (s->points)->next; i=0; j=0;
	      for ( ; p->next != NULL; ) {   /* at least 3 points */
	         fprintf(tfp,"  c%d = .666667[p%d,(%.2lf,%.2lf)];\n",j++,i++,
	            fig2bp(p->x),y_off(fig2bp(p->y)));
	         fprintf(tfp,"  c%d = .666667[p%d,(%.2lf,%.2lf)];\n",j++,i,
	            fig2bp(p->x),y_off(fig2bp(p->y)));
	         p = p->next;
	      }
	      if (s->type == 1) {  /* closed spline */
	         fprintf(tfp,"  c%d = .666667[p%d,(%.2lf,%.2lf)];\n",j++,i++,
	            fig2bp(p->x),y_off(fig2bp(p->y)));
	         fprintf(tfp,"  c%d = .666667[p0,(%.2lf,%.2lf)];\n",j++,
	            fig2bp(p->x),y_off(fig2bp(p->y)));
	      }
/* now draw the spline */
	      p = s->points; i=0; j=0;
	      if (s->type == 1)    /* closed spline */
	         fprintf(tfp,"  s =\n");
	      else
	         fprintf(tfp,"  s = (%.2lf,%.2lf)..\n", fig2bp(p->x),
	         y_off(fig2bp(p->y)));
	      p = p->next;
	      for ( ; p->next != NULL; ) {  /* 3 or more points */
	         fprintf(tfp,"    p%d..controls c%d and c%d..\n",i++,j++,j++);
	         p = p->next;
	      }
	      if (s->type == 1)     /* closed spline */
	         fprintf(tfp,"    p%d..controls c%d and c%d..cycle;\n",i++,j++,j++);
	      else
	         fprintf(tfp,"    p%d..(%.2lf,%.2lf);\n",i,
	            fig2bp(p->x),y_off(fig2bp(p->y)));
	      if (s->fill_style != -1) {   /* Filled? */
	         fprintf(tfp,"  path f;\n");
	         fprintf(tfp,"  f = s--cycle;\n");
	         fprintf(tfp,"  fill f %s;\n",
	            genmp_fillcolor(s->fill_color,s->fill_style));
	      }
	      if (s->thickness != 0) {     /* invisible pen? */
	         fprintf(tfp,"  draw s ");
	         fprintf(tfp,"withcolor %s",genmp_pencolor(s->pen_color));
	         if (s->style == 1) {     /* dashed */
	            fprintf(tfp," dashed evenly scaled %.2lf;\n",s->style_val/3.3);
	         } else if (s->style == 2) {     /* dotted */
	            fprintf(tfp," dashed withdots scaled %.2lf;\n",s->style_val/4.75);
	         } else           /* plain */
	            fprintf(tfp,";\n");
	         if ((s->for_arrow != NULL) || (s->back_arrow != NULL)) {
	            if (s->for_arrow != NULL) {
	               genmp_arrowstats(s->for_arrow);
	               fprintf(tfp,"  forwarr(%d) s ",(s->for_arrow)->style);
	               fprintf(tfp,"withcolor %s;\n",genmp_pencolor(s->pen_color));
	            }
	            if (s->back_arrow != NULL) {
	               genmp_arrowstats(s->back_arrow);
	               fprintf(tfp,"  backarr(%d) s ",(s->back_arrow)->style);
	               fprintf(tfp,"withcolor %s;\n",genmp_pencolor(s->pen_color));
	            }
	         }
	      }
	      break;
	   case 2:         /* interpolated spline (open) */
	   case 3:         /* interpolated spline (closed) */
	      fprintf(tfp,"  linecap:=%d;\n",s->cap_style);
	      fprintf(tfp,"  path s;\n");
	      fprintf(tfp,"  pickup pencircle scaled %.2lf;\n",fig2bp(s->thickness));
	      c = s->controls;
	      p = s->points;
	      fprintf(tfp,"  s = (%.2lf, %.2lf)", fig2bp(p->x),y_off(fig2bp(p->y)));
	      p = p->next;
	      for ( ; p != NULL; p=p->next, c=c->next) {
	         fprintf(tfp,"..controls (%.2lf, %.2lf) and (%.2lf, %.2lf)\n",
	            fig2bp(c->rx), y_off(fig2bp(c->ry)),fig2bp((c->next)->lx),
	            y_off(fig2bp((c->next)->ly)));
	         fprintf(tfp,"  ..(%.2lf,%.2lf)",fig2bp(p->x),y_off(fig2bp(p->y)));
	      }
	      if (s->type == 3)       /* closed spline */
	         fprintf(tfp,"..cycle;\n");
	      else
	         fprintf(tfp,";\n");
	      if (s->fill_style != -1) {   /* Filled? */
	         fprintf(tfp,"  path f;\n");
	         fprintf(tfp,"  f = s--cycle;\n");
	         fprintf(tfp,"  fill f %s;\n",
	            genmp_fillcolor(s->fill_color,s->fill_style));
	      }
	      if (s->thickness != 0) {     /* invisible pen? */
	         fprintf(tfp,"  draw s ");
	         fprintf(tfp,"withcolor %s",genmp_pencolor(s->pen_color));
	         if (s->style == 1) {     /* dashed */
	            fprintf(tfp," dashed evenly scaled %.2lf;\n",s->style_val/3.3);
	         } else if (s->style == 2) {     /* dotted */
	            fprintf(tfp," dashed withdots scaled %.2lf;\n",s->style_val/4.75);
	         } else           /* plain */
	            fprintf(tfp,";\n");
	         if ((s->for_arrow != NULL) || (s->back_arrow != NULL)) {
	            if (s->for_arrow != NULL) {
	               genmp_arrowstats(s->for_arrow);
	               fprintf(tfp,"  forwarr(%d) s ",(s->for_arrow)->style);
	               fprintf(tfp,"withcolor %s;\n",genmp_pencolor(s->pen_color));
	            }
	            if (s->back_arrow != NULL) {
	               genmp_arrowstats(s->back_arrow);
	               fprintf(tfp,"  backarr(%d) s ",(s->back_arrow)->style);
	               fprintf(tfp,"withcolor %s;\n",genmp_pencolor(s->pen_color));
	            }
	         }
	      }
	      break;
	   default:
	      fprintf(tfp,"  show \"This Spline object is not supported!\"\n");
	}
	fprintf(tfp,"%% End spline object\n");
	return;
}


void
genmp_ellipse(e)
F_ellipse *e;
{

        do_split(e->depth);

	/* print any comments prefixed with "%" */
	print_comments("% ",e->comments, "");

	fprintf(tfp,"%% Begin ellipse object\n");
	switch(e->type) {
	   case 1:         /* Ellipse by radius */
	   case 2:         /* Ellipse by diameter */
	   case 3:         /* Circle by radius */
	   case 4:         /* Circle by diameter */
	      fprintf(tfp,"  pickup pencircle scaled %.2lf;\n",fig2bp(e->thickness));
	      fprintf(tfp,"  path c;\n");
	      fprintf(tfp,"  c = fullcircle scaled %.2lf yscaled %.2lf\n",
	         fig2bp((e->radiuses).x*2),
	         fig2bp((e->radiuses).y)/fig2bp((e->radiuses).x));
	      fprintf(tfp,"         rotated %.2lf shifted (%.2lf,%.2lf);\n",
	         rad2deg(e->angle),fig2bp((e->center).x),
	         y_off(fig2bp((e->center).y)));
	      if (e->fill_style != -1)
	         fprintf(tfp," fill c %s;\n",
	            genmp_fillcolor(e->fill_color,e->fill_style));
	      if (e->thickness != 0) {     /* invisible pen? */
	         fprintf(tfp,"  draw c withcolor %s",
	            genmp_pencolor(e->pen_color));
	         if (e->style == 1) {     /* dashed */
	            fprintf(tfp," dashed evenly scaled %.2lf;\n",e->style_val/3.3);
	         } else if (e->style == 2) {     /* dotted */
	            fprintf(tfp," dashed withdots scaled %.2lf;\n",e->style_val/4.75);
	         } else {         /* plain */
	            fprintf(tfp,";\n");
	         }
	      }
	      break;
	   default:
	      fprintf(tfp,"  show \"This Ellipse object is not supported!\"\n");
	}
	fprintf(tfp,"%% End ellipse object\n");
	return;
}

void
genmp_arc(a)
F_arc *a;
{

        do_split(a->depth);

	/* print any comments prefixed with "%" */
	print_comments("% ",a->comments, "");

	fprintf(tfp,"%% Begin arc object\n");
	switch (a->type) {
	   case 1:            /* three point arc (open) */
	   case 2:            /* three point arc (pie wedge) */
	      fprintf(tfp,"  linecap:=%d;\n",a->cap_style);
	      fprintf(tfp,"  linejoin:=0;\n");  /* mitered necessary for pie wedge */
	      fprintf(tfp,"  pickup pencircle scaled %.2lf;\n",fig2bp(a->thickness));
	      fprintf(tfp,"  path a,p,ls,le;\n");
	      fprintf(tfp,"  pair s,e,c;\n");
	      fprintf(tfp,"  c = (%.2lf,%.2lf);\n",fig2bp(a->center.x),
	         y_off(fig2bp(a->center.y)));
	      fprintf(tfp,"  s = (%.2lf,%.2lf);\n",fig2bp(a->point[0].x),
	         y_off(fig2bp(a->point[0].y)));
	      fprintf(tfp,"  e = (%.2lf,%.2lf);\n",fig2bp(a->point[2].x),
	         y_off(fig2bp(a->point[2].y)));
	      fprintf(tfp,"  d := (%.2lf ++ %.2lf)*2.0;\n",
	         fig2bp(a->point[0].x)-fig2bp(a->center.x),
	         fig2bp(a->point[0].y)-fig2bp(a->center.y));
	      fprintf(tfp,"  ls = (0,0)--(d,0) rotated angle (s-c);\n");
	      fprintf(tfp,"  le = (0,0)--(d,0) rotated angle (e-c);\n");
	      fprintf(tfp,"  p = ");
	      if (a->direction != 1)  /* clockwise */
	         fprintf(tfp,"reverse ");
	      fprintf(tfp,"fullcircle scaled d rotated angle (s-c) ");
	      fprintf(tfp,"cutafter le;\n");
	      if (a->type == 2)    /* pie wedge */
	         fprintf(tfp,"  a = buildcycle(ls,p,le) shifted c;\n");
	      else
	         fprintf(tfp,"  a = p shifted c;\n");
	      if (a->fill_style != -1) {   /* Filled arc */
	         fprintf(tfp,"  path f;\n");
	         fprintf(tfp,"  f = a--cycle;\n");
	         fprintf(tfp,"  fill f %s;\n",
	            genmp_fillcolor(a->fill_color,a->fill_style));
	      }
	      if (a->thickness != 0) {     /* invisible pen? */
	         fprintf(tfp,"  draw a ");
	         fprintf(tfp,"withcolor %s",genmp_pencolor(a->pen_color));
	         if (a->style == 1) {     /* dashed */
	            fprintf(tfp," dashed evenly scaled %.2lf;\n",a->style_val/3.3);
	         } else if (a->style == 2) {     /* dotted */
	            fprintf(tfp," dashed withdots scaled %.2lf;\n",a->style_val/4.75);
	         } else           /* plain */
	            fprintf(tfp,";\n");
	         if ((a->for_arrow != NULL) || (a->back_arrow != NULL)) {
	            if (a->for_arrow != NULL) {
	               genmp_arrowstats(a->for_arrow);
	               fprintf(tfp,"  forwarr(%d) a ",(a->for_arrow)->style);
	               fprintf(tfp,"withcolor %s;\n",genmp_pencolor(a->pen_color));
	            }
	            if (a->back_arrow != NULL) {
	               genmp_arrowstats(a->back_arrow);
	               fprintf(tfp,"  backarr(%d) a ",(a->back_arrow)->style);
	               fprintf(tfp,"withcolor %s;\n",genmp_pencolor(a->pen_color));
	            }
	         }
	      }
	      break;
	   default:
	      fprintf(tfp,"  show \"This Arc object is not supported!\"\n");
	}
	fprintf(tfp,"%% End arc object\n");
	return;
}

void
genmp_text(t)
F_text *t;
{
	char ident[3];
	char fname[FONTNAMESIZE];

	do_split(t->depth);

	/* print any comments prefixed with "%" */
	print_comments("% ",t->comments, "");

	fprintf(tfp,"%% Begin text object\n");
   fprintf(tfp,"  picture p;\n");
/*
 * These next three lines pick a unique name for the selected
 *  font and size for use by TeX when typesetting.
 */
   sprintf(ident,"%c%c",char1(),char2());
   strcpy(fname,getfont(t->flags,t->font)); fname[3] = '\0';
   strcat(fname,ident);

/* Define fonts for TeX */
   genmp_writefontinfo(t->size,getfont(t->flags,t->font),fname);

	fprintf(tfp,"  p = btex \\%s %s etex\n",fname,t->cstring);
	fprintf(tfp,"    rotated %.2lf;\n",rad2deg(t->angle));

	switch( t->type ) {
	  case 0:       /* left justified */
	     fprintf(tfp,"  label.urt(p,(%.2lf,%.2lf)) ",fig2bp(t->base_x),
	        y_off(fig2bp(t->base_y)));
	     fprintf(tfp,"withcolor %s;\n",genmp_pencolor(t->color));
	     break;
	  case 1:	/* centered */
	     fprintf(tfp,"  label.top(p,(%.2lf,%.2lf)) ",fig2bp(t->base_x),
	        y_off(fig2bp(t->base_y)));
	     fprintf(tfp,"withcolor %s;\n",genmp_pencolor(t->color));
	     break;
	  case 2:	/* right justified */
	     fprintf(tfp,"  label.ulft(p,(%.2lf,%.2lf)) ",fig2bp(t->base_x),
	        y_off(fig2bp(t->base_y)));
	     fprintf(tfp,"withcolor %s;\n",genmp_pencolor(t->color));
	     break;
	  default:
	     fprintf(tfp,"  show \"This Text object is not supported!\"\n");
	}
	fprintf(tfp,"%% End text object\n");
	return;
}

char *
genmp_pencolor(c)
int c;
{
	static char p_string[30];
/* It is assumed that c will never be -1 (default) */
	if ((c > 0) && (c < NUM_STD_COLS))    /* normal color */
           strcpy(p_string,xfigcolors[c]);
	else if (c >= NUM_STD_COLS)     /* user defined color */
	   sprintf(p_string,"(%.2lf,%.2lf,%.2lf)",
	      user_colors[c-NUM_STD_COLS].r/255.0,
	      user_colors[c-NUM_STD_COLS].g/255.0,
	      user_colors[c-NUM_STD_COLS].b/255.0);
	else                        /* black or default color */
	   strcpy(p_string,"(0.00,0.00,0.00)");
	return(p_string);
}

char *
genmp_fillcolor(c,s)
int c,s;
{
	static char f_string[100];

	if (c == 0)    /* black fill */
	   sprintf(f_string,"withcolor (black + %.2lfwhite)",
	      (double)(20-s)/20.0);
	else           /* other fill */
	   sprintf(f_string,"withcolor (%s + %.2lfwhite)",
	      genmp_pencolor(c),(double)(s-20)/20.0);
	return(f_string);
}

void
genmp_arrowstats(a)
F_arrow *a;
{
	fprintf(tfp,"  ahlength:=%.2lf;\n",fig2bp(a->ht));
	fprintf(tfp,"  ahangle:=angle (%.2lf,%.2lf) * 2.0;\n",fig2bp(a->ht),
	   fig2bp(a->wid)/2.0);
	return;
}


void
genmp_writefontinfo(psize,font,name)
double psize;
char *font, *name;
{
   double ten, seven, five;

/* For some reason, fonts are bigger than they should be */
	psize *= BPperINCH/FIGSperINCH;

	ten = psize; seven = psize*.7; five = psize*.5;
	fprintf(tfp,"  verbatimtex\n");
	fprintf(tfp,"    \\font\\%s=%s at %.2lfpt\n",name,font,psize);
   fprintf(tfp,"    \\font\\tenrm=cmr10 at %.2lfpt \\font\\sevenrm=cmr7 \
at %.2lfpt\n",ten,seven);
   fprintf(tfp,"    \\font\\fiverm=cmr5 at %.2lfpt \\font\\teni=cmmi10 \
at %.2lfpt\n",five,ten);
   fprintf(tfp,"    \\font\\seveni=cmmi7 at %.2lfpt \\font\\fivei=cmmi5 \
at %.2lfpt\n",seven,five);
   fprintf(tfp,"    \\font\\tensy=cmsy10 at %.2lfpt \\font\\sevensy=cmsy7 \
at %.2lfpt\n",ten,seven);
   fprintf(tfp,"    \\font\\fivesy=cmsy5 at %.2lfpt \\textfont0\\tenrm\n", five);
   fprintf(tfp,"    \\scriptfont0\\sevenrm \\scriptscriptfont0\\fiverm\n");
   fprintf(tfp,"    \\textfont1\\teni \\scriptfont1\\seveni \
\\scriptscriptfont1\\fivei\n");
   fprintf(tfp,"    \\textfont2\\tensy \\scriptfont2\\sevensy \
\\scriptscriptfont2\\fivesy\n");
	fprintf(tfp,"  etex;\n");
}

/*
 * If we are in "split" mode, we must start new figure if the current
 * depth and the last_depth differ by more than one.
 * Depths will be seen with decreasing values.
 */
void
do_split(actual_depth)
int actual_depth;
{
    if (split) {
           if (actual_depth+1 < last_depth) {
	      /* depths differ by more than one */
	      if (fig_number > 0) {
		  /* end the current figure, if we already had one */
		  fprintf(tfp,"setbounds currentpicture to allbounds;\n");
		  fprintf(tfp,"endfig;\n");
	      }
	      /* start a new figure with a comment */
              fprintf(tfp,"%% Now draw objects of depth: %d\n",actual_depth);
	      fprintf(tfp,"beginfig(%d)\n",fig_number++);
	   }
	   last_depth = actual_depth;
        }
}

struct driver dev_mp = {
	genmp_option,
	genmp_start,
	genmp_arc,
	genmp_ellipse,
	genmp_line,
	genmp_spline,
	genmp_text,
	genmp_end,
	INCLUDE_TEXT
};

