/*	BSDI $Id$	*/

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

/* 
 *	FIG : Facility for Interactive Generation of figures
 *
 *	Copyright (c) 1985, 1988 by Supoj Sutanthavibul (supoj@sally.UTEXAS.EDU)
 *	January 1985.
 *	1st revision : August 1985.
 *	2nd revision : March 1988.
 *
 *	%W%	%G%
*/
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <sys/param.h>
#include <string.h>
#include "alloc.h"
#include "fig2dev.h"
#include "object.h"
#include "trans_spline.h"
#include "../patchlevel.h"

#if defined(hpux) || defined(SYSV) || defined(SVR4)
#define bzero(s,n) memset((s),'\0',(n))
#endif

#ifdef NEED_STRERROR
char *
strerror(e)
	int e;
{
#if (! (defined(BSD) && (BSD >= 199306)) && !defined(__NetBSD__))
	extern char *sys_errlist[];
#endif
	return sys_errlist[e];
	}
#endif

extern int            errno;

extern F_arrow		*make_arrow();
extern char		*calloc();
extern double		 floor(), ceil();

static void		 read_colordef();
static F_ellipse	*read_ellipseobject();
static F_line		*read_lineobject();
static F_text		*read_textobject();
static F_spline		*read_splineobject();
static F_arc		*read_arcobject();
static F_compound	*read_compoundobject();

#define			FILL_CONVERT(f) \
				((v2_flag || (f) < WHITE_FILL) \
					? (v30_flag? f: (f-1)) : 20 - ((f)-1)*5)

#define			BUF_SIZE		1024

User_color		user_colors[MAX_USR_COLS];
int			user_col_indx[MAX_USR_COLS];
int			num_usr_cols;
char			buf[BUF_SIZE];
int			line_no = 0;
int			num_object;
int			v2_flag;	/* Protocol V2.0 or higher */
int			v21_flag;	/* Protocol V2.1 or higher */
int			v30_flag;	/* Protocol V3.0 or higher */
int			v32_flag;	/* Protocol V3.2 or higher */
float			THICK_SCALE;	/* convert line thickness from screen res. */

read_fail_message(file, err)
char	*file;
int	err;
{
	if (err == 0)		/* Successful read */
	    return;
#if !defined(hpux) && !defined(SYSV) && !defined(SVR4)
	else if (err == ENAMETOOLONG)
	    put_msg("File name \"%s\" is too long", file);
#endif
	else if (err == ENOENT)
	    put_msg("File \"%s\" does not exist", file);
	else if (err == ENOTDIR)
	    put_msg("A name in the path \"%s\" is not a directory", file);
	else if (err == EACCES)
	    put_msg("Read access to file \"%s\" is blocked", file);
	else if (err == EISDIR)
	    put_msg("File \"%s\" is a directory", file);
	else if (err == -2) {
	    put_msg("File \"%s\" is empty", file);
	    }
	else if (err == -1) {
	    /* Format error; relevant error message is already delivered */
	    }
	else
	    put_msg("File \"%s\" is not accessable; %s", file, strerror(err));
	}

/**********************************************************
Read_fig returns :

     0 : successful read.
    -1 : File is in incorrect format
    -2 : File is empty
err_no : if file can not be read for various reasons

The resolution (ppi) and the cooridnate system (coord_sys) are
stored in obj->nwcorner.x and obj->nwcorner.x respectively.
**********************************************************/

read_fig(file_name, obj)
char		*file_name;
F_compound	*obj;
{
	FILE		*fp;

	init_pats_used();
	if ((fp = fopen(file_name, "r")) == NULL)
	    return(errno);
	else
	    return(readfp_fig(fp, obj));
	}

readfp_fig(fp, obj)
FILE	*fp;
F_compound	*obj;
{
	char		c;
	int		status;

	num_object = 0;
	num_usr_cols = 0;
	c = fgetc(fp);
	if (feof(fp)) return(-2);
	ungetc(c, fp);
	bzero((char*)obj, COMOBJ_SIZE);
	if (c == '#')
	    status = read_objects(fp, obj);
	else
	    status = read_1_3_objects(fp, obj);
	(void)fclose(fp);
	return(status);
	}
	
int
read_objects(fp, obj)
FILE		*fp;
F_compound	*obj;
{
	F_ellipse	*e, *le = NULL;
	F_line		*l, *ll = NULL;
	F_text		*t, *lt = NULL;
	F_spline	*s, *ls = NULL;
	F_arc		*a, *la = NULL;
	F_compound	*c, *lc = NULL;
	int		object, ppi, coord_sys;
	int		metric;

	bzero((char*)obj, COMOBJ_SIZE);
	(void)fgets(buf, BUF_SIZE, fp);	/* get the version line */
	if (strlen(buf) > (size_t)0)
	    buf[strlen(buf)-1] = '\0';	/* remove newline */

	/* v2_flag is for version 2 or higher */
	v2_flag = (!strncmp(buf, "#FIG 2", 6) || !strncmp(buf, "#FIG 3", 6));
	/* v21_flag is for version 2.1 or higher */
	v21_flag = (!strncmp(buf, "#FIG 2.1", 8) || !strncmp(buf, "#FIG 3", 6));
	/* version 2.2 was only beta - 3.0 is the official release (they are identical) */
	v30_flag = (!strncmp(buf, "#FIG 3", 6) || !strncmp(buf, "#FIG 2.2", 8));
	/* version 3.2 contains paper size, magnif, multiple page and transparent color
	   in Fig file */
	v32_flag = (!strncmp(buf, "#FIG 3.2", 8));
	if (strncmp(&buf[5],VERSION,3) > 0) {
	    put_msg("Fig file format (%s) newer than this version of fig2dev (%s), exiting",
			&buf[5],VERSION);
	    exit(1);
	}

	if (v30_flag) {
	    /* read the orientation spec (landscape/portrait) */
	    line_no++;
	    if (get_line(fp) < 0) {
		put_msg("File is truncated at landscape/portrait specification.");
		return(-1);
	    }
	    /* but set only if the user didn't specify the orientation
	       on the command line */
	    if (!orientspec)
		landscape = !strncasecmp(buf,"land",4);

	    /* now read the metric/inches spec OR centering spec */
	    line_no++;
	    if (get_line(fp) < 0) {
		put_msg("File is truncated at metric/inches or centering specification.");
		return(-1);
	    }
	    /* read justification spec */
	    if ((strncasecmp(buf,"center",6) == 0) || 
		(strncasecmp(buf,"flush",5) == 0)) {
		/* but set only if user didn't specify it */
		if (!centerspec)
		    center = strncasecmp(buf,"flush",5);
		/* now read metric/inches spec */
		line_no++;
		if (get_line(fp) < 0) {
		    put_msg("File is truncated at metric/inches specification.");
		    return(-1);
		}
	    }
	
	    /* new stuff in 3.2 */
	    if (v32_flag) {
		char *p;
		/* read the paper size */
		line_no++;
		if (get_line(fp) < 0) {
		    put_msg("File is truncated at paper size specification.");
		    return(-1);
		}
		if (!paperspec) {
		    /* copy all except newline */
		    strncpy(papersize,buf,strlen(buf)-1);
		    /* and truncate at first blank, if any */
		    if (p=strchr(papersize,' '))
			*p = '\0';
		}

		/* read the magnification */
		line_no++;
		if (get_line(fp) < 0) {
		    put_msg("File is truncated at magnification specification.");
		    return(-1);
		}
		if (!magspec)
		    mag = atof(buf)/100.0;

		/* read the multiple page flag */
		line_no++;
		if (get_line(fp) < 0) {
		    put_msg("File is truncated at multiple page specification.");
		    return(-1);
		}
		if (!multispec)
		    multi_page = (strncasecmp(buf,"multiple",8) == 0);

		/* read the transparent color.  This is ignored because it is
		   only used by xfig when exporting to GIF */
		line_no++;
		if (get_line(fp) < 0) {
		    put_msg("File is truncated at transparent color specification.");
		    return(-1);
		}
	    } 
	    /* if metric, scale magnification to correct for xfig display error */
	    if (strncasecmp(buf,"metric",5)==0) {
		metric = 1;
		mag *= 80.0/76.2;
	    } else {
		metric = 0;
	    }
	} else {
	/* older than 3.1, no metric/inch flag */
	    metric = 0;
	}

	/* v 3.1 or older, set paper size unless user asked with -z */
	if (!v32_flag) {
	    if (!paperspec) {
		if (metric)
		    strcpy(papersize,"A4");
		else
		    strcpy(papersize,"Letter");
	    }
	}

	line_no++;
	/* now read for resolution and coord_sys */
	if (get_line(fp) < 0) {
	    put_msg("File is truncated at resolution specification.");
	    return(-1);
	    }
	if (sscanf(buf,"%d%d\n", &ppi, &coord_sys) != 2) {
	    put_msg("Incomplete resolution information at line %d", line_no);
	    return(-1);
	    }

	THICK_SCALE = ppi/80;	/* convert line thickness from screen resolution */
	obj->nwcorner.x = ppi;
	obj->nwcorner.y = coord_sys;
	while (get_line(fp) > 0) {
	    if (sscanf(buf, "%d", &object) != 1) {
		put_msg("Incorrect format at line %d", line_no);
		return(-1);
		}
	    switch (object) {
		case O_COLOR_DEF:
		    read_colordef(fp);
		    if (num_object) {
			put_msg("Color definitions must come before other objects (line %d).",
				line_no);
			return (-1);
		    }
		    num_usr_cols++;
		    break;
		case O_POLYLINE :
		    if ((l = read_lineobject(fp)) == NULL) 
			return(-1);
		    if (ll)
			ll = (ll->next = l);
		    else 
			ll = obj->lines = l;
		    num_object++;
		    break;
		case O_SPLINE :
		    if ((s = read_splineobject(fp)) == NULL) { 
			return(-1);
			}
		    if (v32_flag){ /* s is a line */
		      if (ll)
			ll = (ll->next = (F_line *) s);
		      else 
			ll = obj->lines = (F_line *) s;
		      num_object++;
		      break;		      
		    }
		    if (ls)
			ls = (ls->next = s);
		    else 
			ls = obj->splines = s;
		    num_object++;
		    break;
		case O_ELLIPSE :
		    if ((e = read_ellipseobject()) == NULL) 
			return(-1);
		    if (le)
			le = (le->next = e);
		    else 
			le = obj->ellipses = e;
		    num_object++;
		    break;
		case O_ARC :
		    if ((a = read_arcobject(fp)) == NULL) 
			return(-1);
		    if (la)
			la = (la->next = a);
		    else 
			la = obj->arcs = a;
		    num_object++;
		    break;
		case O_TEXT :
		    if ((t = read_textobject(fp)) == NULL) 
			return(-1);
		    if (lt)
			lt = (lt->next = t);
		    else 
			lt = obj->texts = t;
		    num_object++;
		    break;
		case O_COMPOUND :
		    if ((c = read_compoundobject(fp)) == NULL) 
			return(-1);
		    if (lc)
			lc = (lc->next = c);
		    else 
			lc = obj->compounds = c;
		    num_object++;
		    break;
		default :
		    put_msg("Incorrect object code at line %d", line_no);
		    return(-1);
		} /*  switch */
	    } /*  while */
	if (feof(fp))
	    return(0);
	else
	    return(errno);
	} /*  read_objects */

static void
read_colordef(fp)
    FILE	   *fp;
{ 
    int		    c,r,g,b;

    if ((sscanf(buf, "%*d %d #%02x%02x%02x", &c, &r, &g, &b) != 4) ||
		(c < NUM_STD_COLS)) {
	buf[strlen(buf)-1]='\0';	/* remove the newline */
	put_msg("Invalid color definition: %s, setting to black (#00000).",buf);
	r=g=b=0;
    }
    user_col_indx[num_usr_cols] = c;
    user_colors[num_usr_cols].r = r;
    user_colors[num_usr_cols].g = g;
    user_colors[num_usr_cols].b = b;
}

fix_color(color)
    int		    *color;
{
    int		    i;
    if (*color < NUM_STD_COLS)
	return;
    for (i=0; i<num_usr_cols; i++)
	if (*color == user_col_indx[i]) {
		*color = i+NUM_STD_COLS;
		return;
	}
    put_msg("Cannot locate user color %d, using default color for line %d.",
		*color,line_no);
    *color = DEFAULT;
    return;
}

static F_arc *
read_arcobject(fp)
FILE	*fp;
{
	F_arc	*a;
	int	n, fa, ba;
	int	type, style;
	double	thickness, wid, ht;

	if (NULL == (Arc_malloc(a))) {
	    put_msg(Err_mem);
	    return(NULL);
	    }
	a->pen = 0;
	a->fill_style = 0;
	a->for_arrow = NULL;
	a->back_arrow = NULL;
	a->next = NULL;
	if (v30_flag) {
	    n = sscanf(buf, "%*d%d%d%d%d%d%d%d%d%lf%d%d%d%d%lf%lf%d%d%d%d%d%d\n",
		&a->type, &a->style, &a->thickness, 
		&a->pen_color, &a->fill_color, &a->depth, &a->pen, &a->fill_style, 
		&a->style_val, &a->cap_style,
		&a->direction, &fa, &ba,
		&a->center.x, &a->center.y, 
		&a->point[0].x, &a->point[0].y, 
		&a->point[1].x, &a->point[1].y, 
		&a->point[2].x, &a->point[2].y);
	} else {
	    n = sscanf(buf, "%*d%d%d%d%d%d%d%d%lf%d%d%d%lf%lf%d%d%d%d%d%d\n",
		&a->type, &a->style, &a->thickness, 
		&a->pen_color, &a->depth, &a->pen, &a->fill_style, 
		&a->style_val, &a->direction, &fa, &ba,
		&a->center.x, &a->center.y, 
		&a->point[0].x, &a->point[0].y, 
		&a->point[1].x, &a->point[1].y, 
		&a->point[2].x, &a->point[2].y);
	    a->fill_color = a->pen_color;
	    a->cap_style = 0;        /* butt line cap */
	}
	if ((v30_flag && n != 21) || (!v30_flag && n != 19)) {
	    put_msg(Err_incomp, "arc", line_no);
	    free((char*)a);
	    return(NULL);
	    }
	a->thickness *= THICK_SCALE;
	a->fill_style = FILL_CONVERT(a->fill_style);
	/* keep track if pattern is used */
	note_pattern(a->fill_style);
	fix_color(&a->pen_color);
	fix_color(&a->fill_color);
	skip_comment(fp);
	if (fa) {
	    line_no++;
	    if (fscanf(fp, "%d%d%lf%lf%lf", &type, &style, &thickness, &wid, &ht) != 5) {
		fprintf(stderr, Err_incomp, "arc", line_no);
		return(NULL);
		}
	    skip_line(fp);
	    a->for_arrow = make_arrow(type, style, thickness, wid, ht);
	    skip_comment(fp);
	    }
	skip_comment(fp);
	if (ba) {
	    line_no++;
	    if (fscanf(fp, "%d%d%lf%lf%lf", &type, &style, &thickness, &wid, &ht) != 5) {
		fprintf(stderr, Err_incomp, "arc", line_no);
		return(NULL);
		}
	    skip_line(fp);
	    a->back_arrow = make_arrow(type, style, thickness, wid, ht);
	    }
	return(a);
	}

static F_compound *
read_compoundobject(fp)
FILE	*fp;
{
	F_arc		*a, *la = NULL;
	F_ellipse	*e, *le = NULL;
	F_line		*l, *ll = NULL;
	F_spline	*s, *ls = NULL;
	F_text		*t, *lt = NULL;
	F_compound	*com, *c, *lc = NULL;
	int		n, object;

	Compound_malloc(com);
	com->arcs = NULL;
	com->ellipses = NULL;
	com->lines = NULL;
	com->splines = NULL;
	com->texts = NULL;
	com->compounds = NULL;
	com->next = NULL;
	n = sscanf(buf, "%*d%d%d%d%d\n", &com->nwcorner.x, &com->nwcorner.y,
		&com->secorner.x, &com->secorner.y);
	if (n != 4) {
	    put_msg(Err_incomp, "compound", line_no);
	    free((char*)com);
	    return(NULL);
	    }
	while (get_line(fp) > 0) {
	    if (sscanf(buf, "%d", &object) != 1) {
		put_msg(Err_incomp, "compound", line_no);
		free_compound(&com);
		return(NULL);
		}
	    switch (object) {
		case O_POLYLINE :
		    if ((l = read_lineobject(fp)) == NULL) { 
			free_line(&l);
			return(NULL);
			}
		    if (ll)
			ll = (ll->next = l);
		    else 
			ll = com->lines = l;
		    break;
		case O_SPLINE :
		    if ((s = read_splineobject(fp)) == NULL) { 
			free_spline(&s);
			return(NULL);
			}
		    if (v32_flag){ /* s is a line */
		      if (ll)
			ll = (ll->next = (F_line *) s);
		      else 
			ll = com->lines = (F_line *) s;
		      break;		      
		    }
		    if (ls)
			ls = (ls->next = s);
		    else 
			ls = com->splines = s;
		    break;
		case O_ELLIPSE :
		    if ((e = read_ellipseobject()) == NULL) { 
			free_ellipse(&e);
			return(NULL);
			}
		    if (le)
			le = (le->next = e);
		    else 
			le = com->ellipses = e;
		    break;
		case O_ARC :
		    if ((a = read_arcobject(fp)) == NULL) { 
			free_arc(&a);
			return(NULL);
			}
		    if (la)
			la = (la->next = a);
		    else 
			la = com->arcs = a;
		    break;
		case O_TEXT :
		    if ((t = read_textobject(fp)) == NULL) { 
			free_text(&t);
			return(NULL);
			}
		    if (lt)
			lt = (lt->next = t);
		    else 
			lt = com->texts = t;
		    break;
		case O_COMPOUND :
		    if ((c = read_compoundobject(fp)) == NULL) { 
			free_compound(&c);
			return(NULL);
			}
		    if (lc)
			lc = (lc->next = c);
		    else 
			lc = com->compounds = c;
		    break;
		case O_END_COMPOUND :
		    return(com);
		default :
		    put_msg("Wrong object code at line %d", line_no);
		    return(NULL);
		} /*  switch */
	    }
	if (feof(fp))
	    return(com);
	else
	    return(NULL);
	}

static F_ellipse *
read_ellipseobject()
{
	F_ellipse	*e;
	int		n;

	Ellipse_malloc(e);
	e->fill_style = 0;
	e->pen = 0;
	e->next = NULL;
	if (v30_flag) {
	    n = sscanf(buf, "%*d%d%d%d%d%d%d%d%d%lf%d%lf%d%d%d%d%d%d%d%d\n",
		&e->type, &e->style, &e->thickness,
		&e->pen_color, &e->fill_color, &e->depth, &e->pen, &e->fill_style,
		&e->style_val, &e->direction, &e->angle,
		&e->center.x, &e->center.y, 
		&e->radiuses.x, &e->radiuses.y, 
		&e->start.x, &e->start.y, 
		&e->end.x, &e->end.y);
	} else {
	    n = sscanf(buf, "%*d%d%d%d%d%d%d%d%lf%d%lf%d%d%d%d%d%d%d%d\n",
		&e->type, &e->style, &e->thickness,
		&e->pen_color, &e->depth, &e->pen, &e->fill_style,
		&e->style_val, &e->direction, &e->angle,
		&e->center.x, &e->center.y, 
		&e->radiuses.x, &e->radiuses.y, 
		&e->start.x, &e->start.y, 
		&e->end.x, &e->end.y);
	    e->fill_color = e->pen_color;
	}
	if ((v30_flag && n != 19) || (!v30_flag && n != 18)) {
	    put_msg(Err_incomp, "ellipse", line_no);
	    free((char*)e);
	    return(NULL);
	    }
	fix_color(&e->pen_color);
	fix_color(&e->fill_color);
	e->thickness *= THICK_SCALE;
	e->fill_style = FILL_CONVERT(e->fill_style);
	/* keep track if pattern is used */
	note_pattern(e->fill_style);
	return(e);
	}

static F_line *
read_lineobject(fp)
FILE	*fp;
{
	F_line	*l;
	F_point	*p, *q;
	int	n, x, y, fa, ba;
	int	type, style, radius_flag;
	double	thickness, wid, ht;
	int	npts;

	Line_malloc(l);
	l->points = NULL;
	l->pen = 0;
	l->fill_style = 0;
	l->for_arrow = NULL;
	l->back_arrow = NULL;
	l->next = NULL;
	l->join_style = 0;
	l->cap_style = 0;        /* butt line cap */

	sscanf(buf,"%*d%d",&l->type);	/* get the line type */

	radius_flag = v30_flag || v21_flag || (v2_flag && l->type == T_ARC_BOX);
	if (radius_flag) {
	    if (v30_flag) {
		n = sscanf(buf, "%*d%d%d%d%d%d%d%d%d%lf%d%d%d%d%d%d",
		&l->type,&l->style,&l->thickness,&l->pen_color,&l->fill_color,
		&l->depth,&l->pen,&l->fill_style,&l->style_val,
		&l->join_style,&l->cap_style,
		&l->radius,&fa,&ba,&npts);
	    } else {
		n = sscanf(buf, "%*d%d%d%d%d%d%d%d%lf%d%d%d",
		&l->type,&l->style,&l->thickness,&l->pen_color,
		&l->depth,&l->pen,&l->fill_style,&l->style_val,&l->radius,&fa, &ba);
		l->fill_color = l->pen_color;
	    }
	}
	/* old format uses pen for radius of arc-box corners */
	else
	    {
	    n = sscanf(buf, "%*d%d%d%d%d%d%d%d%lf%d%d",
			&l->type,&l->style,&l->thickness,&l->pen_color,
			&l->depth,&l->pen,&l->fill_style,&l->style_val,&fa,&ba);
	    l->fill_color = l->pen_color;
	    if (l->type == T_ARC_BOX)
		{
		l->radius = (int) l->pen;
		l->pen = 0;
		}
	    else
		l->radius = 0;
	    }
	if ((!radius_flag && n!=10) ||
	     (radius_flag && ((!v30_flag && n!=11)||(v30_flag && n!=15)))) {
	    put_msg(Err_incomp, "line", line_no);
	    free((char*)l);
	    return(NULL);
	    }
	l->radius *= THICK_SCALE;
	l->thickness *= THICK_SCALE;
	l->fill_style = FILL_CONVERT(l->fill_style);
	/* keep track if pattern is used */
	note_pattern(l->fill_style);
	fix_color(&l->pen_color);
	fix_color(&l->fill_color);
	skip_comment(fp);
	if (fa) {
	    line_no++;
	    if (fscanf(fp, "%d%d%lf%lf%lf", &type, &style, &thickness, &wid, &ht) != 5) {
		fprintf(stderr, Err_incomp, "line", line_no);
		return(NULL);
		}
	    skip_line(fp);
	    l->for_arrow = make_arrow(type, style, thickness, wid, ht);
	    skip_comment(fp);
	    }
	if (ba) {
	    line_no++;
	    if (fscanf(fp, "%d%d%lf%lf%lf", &type, &style, &thickness, &wid, &ht) != 5) {
		fprintf(stderr, Err_incomp, "line", line_no);
		return(NULL);
		}
	    skip_line(fp);
	    l->back_arrow = make_arrow(type, style, thickness, wid, ht);
	    skip_comment(fp);
	    }
    	if (l->type == T_PIC_BOX) {
		line_no++;
		Pic_malloc(l->pic);
		if (l->pic  == NULL) {
		    free((char *) l);
		    return (NULL);
		}
		if (fscanf(fp, "%d %s", &l->pic->flipped, l->pic->file) != 2) {
	    		put_msg(Err_incomp,
				"Picture object", line_no);
	    		fprintf(stderr, Err_incomp,
				"Picture object", line_no);
	    	return (NULL);
		}
    	} else
		l->pic = NULL;

	if (NULL == (l->points = Point_malloc(p))) {
	    put_msg(Err_mem);
	    return(NULL);
	    }
	p->next = NULL;
	if (fscanf(fp, "%d%d", &p->x, &p->y) != 2) {
	    put_msg(Err_incomp, "line", line_no);
	    free_linestorage(l);
	    return(NULL);
	    }
	if (!v30_flag)
		npts = 1000000;
	for (--npts; npts > 0; npts--) {
	    if (fscanf(fp, "%d%d", &x, &y) != 2) {
		put_msg(Err_incomp, "line", line_no);
		free_linestorage(l);
		return(NULL);
		}
	    if (!v30_flag && x == 9999) 
		break;
	    if (NULL == (Point_malloc(q))) {
		put_msg(Err_mem);
		free_linestorage(l);
		return(NULL);
		}
	    q->x = x;
	    q->y = y;
	    q->next = NULL;
	    p->next = q;
	    p = q;
	    }
	skip_line(fp);
	return(l);
	}

static F_spline *
read_splineobject(fp)
FILE	*fp;
{
	F_spline	*s;
	F_line          *l;
	F_point		*p, *q;
	F_control	*cp, *cq;
	int		c, n, x, y, fa, ba;
	int		type, style, npts;
	double		thickness, wid, ht;
	double		lx, ly, rx, ry;

	Spline_malloc(s);
	s->points = NULL;
	s->controls = NULL;
	s->pen = 0;
	s->fill_style = 0;
	s->for_arrow = NULL;
	s->back_arrow = NULL;
	s->next = NULL;

	if (v30_flag) {
	    n = sscanf(buf, "%*d%d%d%d%d%d%d%d%d%lf%d%d%d%d",
	    	&s->type, &s->style, &s->thickness,
		&s->pen_color, &s->fill_color,
		&s->depth, &s->pen, &s->fill_style, &s->style_val,
		&s->cap_style, &fa, &ba, &npts);
	} else {
	    n = sscanf(buf, "%*d%d%d%d%d%d%d%d%lf%d%d",
	    	&s->type, &s->style, &s->thickness, &s->pen_color,
		&s->depth, &s->pen, &s->fill_style, &s->style_val, &fa, &ba);
	    s->fill_color = s->pen_color;
	    s->cap_style = 0;        /* butt line cap */
	}
	if ((v30_flag && n != 13) || (!v30_flag && n != 10)) {
	    put_msg(Err_incomp, "spline", line_no);
	    free((char*)s);
	    return(NULL);
	    }
	s->thickness *= THICK_SCALE;
	s->fill_style = FILL_CONVERT(s->fill_style);
	/* keep track if pattern is used */
	note_pattern(s->fill_style);
	fix_color(&s->pen_color);
	fix_color(&s->fill_color);
	skip_comment(fp);
	if (fa) {
	    line_no++;
	    if (fscanf(fp, "%d%d%lf%lf%lf", &type, &style, &thickness, &wid, &ht) != 5) {
		fprintf(stderr, Err_incomp, "spline", line_no);
		return(NULL);
		}
	    skip_line(fp);
	    s->for_arrow = make_arrow(type, style, thickness, wid, ht);
	    skip_comment(fp);
	    }
	if (ba) {
	    line_no++;
	    if (fscanf(fp, "%d%d%lf%lf%lf", &type, &style, &thickness, &wid, &ht) != 5) {
		fprintf(stderr, Err_incomp, "spline", line_no);
		return(NULL);
		}
	    skip_line(fp);
	    s->back_arrow = make_arrow(type, style, thickness, wid, ht);
	    skip_comment(fp);
	    }

	/* Read points */
	if ((n = fscanf(fp, "%d%d", &x, &y)) != 2) {
	    put_msg(Err_incomp, "spline", line_no);
	    free_splinestorage(s);
	    return(NULL);
	    };
	if (NULL == (s->points = Point_malloc(p))) {
	    put_msg(Err_mem);
	    free_splinestorage(s);
	    return(NULL);
	    }
	p->x = x; p->y = y;
	c = 1;
	if (!v30_flag)
		npts = 1000000;
	for (--npts; npts > 0; npts--) {
	    if (fscanf(fp, "%d%d", &x, &y) != 2) {
		put_msg(Err_incomp, "spline", line_no);
		p->next = NULL;
		free_splinestorage(s);
		return(NULL);
		};
	    if (!v30_flag && x == 9999) 
		break;
	    if (NULL == (Point_malloc(q))) {
		put_msg(Err_mem);
		free_splinestorage(s);
		return(NULL);
		}
	    q->x = x;
	    q->y = y;
	    p->next = q;
	    p = q;
	    c++;
	    }
	p->next = NULL;
	skip_line(fp);

	if (v32_flag)
	  {
	    /* transform x-splines into lines */

	    F_control * ptr;
	    double control_s;

	    make_control_factors(s);
	    ptr = s->controls;
	    while (ptr)    /* read controls */
	      {
		if ((n = fscanf(fp, "%lf", &control_s)) != 1) {
		  put_msg(Err_incomp, "spline", line_no);
		  free_splinestorage(s);
		  return(NULL);
		}
		ptr->s = control_s;
		ptr = ptr->next;
	      }

	    l = create_line_with_spline(s);

	    
	    free_splinestorage(s);  
	    return((F_spline *)l);   /* return the new line */
	  }

	if (approx_spline(s)) return(s);
	skip_comment(fp);
	/* Read controls from older versions */
	if ((n = fscanf(fp, "%lf%lf%lf%lf", &lx, &ly, &rx, &ry)) != 4) {
	    put_msg(Err_incomp, "spline", line_no);
	    free_splinestorage(s);
	    return(NULL);
	    };
	if (NULL == (s->controls = Control_malloc(cp))) {
	    put_msg(Err_mem);
	    free_splinestorage(s);
	    return(NULL);
	    }
	cp->lx = lx; cp->ly = ly;
	cp->rx = rx; cp->ry = ry;
	while (--c) {
	    if (fscanf(fp, "%lf%lf%lf%lf", &lx, &ly, &rx, &ry) != 4) {
		put_msg(Err_incomp, "spline", line_no);
		cp->next = NULL;
		free_splinestorage(s);
		return(NULL);
		};
	    if (NULL == (Control_malloc(cq))) {
		put_msg(Err_mem);
		cp->next = NULL;
		free_splinestorage(s);
		return(NULL);
		}
	    cq->lx = lx; cq->ly = ly;
	    cq->rx = rx; cq->ry = ry;
	    cp->next = cq;
	    cp = cq;
	    }
	cp->next = NULL;

	skip_line(fp);
	return(s);
	}

/* strncasecmp and strcasecmp by Fred Appelman (Fred.Appelman@cv.ruu.nl) */

#ifdef HAVE_NO_STRNCASECMP

int strncasecmp(const char* s1, const char* s2, int n)
{
   char c1,c2;

   while (--n>=0)
   {
          /* Check for end of string, if either of the strings
           * is ended, we can terminate the test
           */
          if (*s1=='\0' && s2!='\0') return -1; /* s1 ended premature */
          if (*s1!='\0' && s2=='\0') return +1; /* s2 ended premature */

          c1=toupper(*s1++);
          c2=toupper(*s2++);
          if (c1<c2) return -1; /* s1 is "smaller" */
          if (c1>c2) return +1; /* s2 is "smaller" */
   }
   return 0;
}

#endif

#ifdef HAVE_NO_STRCASECMP
int strcasecmp(const char* s1, const char* s2)
{
   char c1,c2;

   while (*s1 && *s2)
   {
          c1=toupper(*s1++);
          c2=toupper(*s2++);
          if (c1<c2) return -1; /* s1 is "smaller" */
          if (c1>c2) return +1; /* s2 is "smaller" */
   }
   /* Check for end of string, if not both the strings ended they are
    * not the same.
        */
   if (*s1=='\0' && s2!='\0') return -1; /* s1 ended premature */
   if (*s1!='\0' && s2=='\0') return +1; /* s2 ended premature */
   return 0;
}

#endif
 
static F_text *
read_textobject(fp)
FILE	*fp;
{
	F_text	*t;
	int	n, ignore = 0;
	char	s[BUF_SIZE], s_temp[BUF_SIZE], junk[2];
	int	more, len, l;

	Text_malloc(t);
	t->font = 0;
	t->size = 0.0;
	t->next = NULL;

/* linux can't read 8-bit characters */
#ifdef linux
    {
	char replaced;
	int pos;
	pos = 0;
	len = strlen(buf);
	while (((unsigned char) buf[pos] <= 'e' ) &&
		((unsigned char) buf[pos] >= ' ' ) && buf[pos] )
			pos++;
	replaced = buf[pos];
	buf[pos]='f';
	if (v30_flag) {	/* order of parms is more like other objects now */
	    n = sscanf(buf, "%*d%d%d%d%d%d%lf%lf%d%lf%lf%d%d%[^f]%[f]",
		&t->type, &t->color, &t->depth, &t->pen,
		&t->font, &t->size, &t->angle,
		&t->flags, &t->height, &t->length,
		&t->base_x, &t->base_y, s, junk);
	} else {
	    n = sscanf(buf, "%*d%d%d%lf%d%d%d%lf%d%lf%lf%d%d%[^f]%[f]",
		&t->type, &t->font, &t->size, &t->pen,
		&t->color, &t->depth, &t->angle,
		&t->flags, &t->height, &t->length,
		&t->base_x, &t->base_y, s, junk);
	}
	n--;
	if ( n < 13 ) {
	    put_msg(Err_incomp, "text", line_no);
	    free((char *) t);
	    return (NULL);
	   }
	buf[pos]=replaced;
	strcpy( s, buf+pos-strlen(s));
	len=strlen(s);
	if ( len && (s[len-1] ==  '\n') )
	    s[len-1]='\0';
	len=strlen(s);
	if (!v30_flag) {		/* if older version, remove the ^A */
	    if ( len && (s[len-1] ==  1) ) {
		n++;
		s[len-1]='\0';
	    }
	}
    }

#else
	if (v30_flag) {	/* order of parms is more like other objects now,
			   string is now terminated with the literal '\001',
			   and 8-bit characters are represented as \xxx */

	    n = sscanf(buf, "%*d%d%d%d%d%d%lf%lf%d%lf%lf%d%d%[^\n]",
		&t->type, &t->color, &t->depth, &t->pen,
		&t->font, &t->size, &t->angle,
		&t->flags, &t->height, &t->length,
		&t->base_x, &t->base_y, s, junk);
	} else {
	    /* The text object is terminated by a CONTROL-A, so we read
		everything up to the CONTROL-A and then read that character.
		If we do not find the CONTROL-A on this line then this must
		be a multi-line text object and we will have to read more. */

	    n = sscanf(buf,"%*d%d%d%lf%d%d%d%lf%d%lf%lf%d%d%[^\1]%[\1]",
		&t->type, &t->font, &t->size, &t->pen,
		&t->color, &t->depth, &t->angle,
		&t->flags, &t->height, &t->length, 
		&t->base_x, &t->base_y, s, junk);
	}
	if ((n != 14) && (n != 13)) {
	  put_msg(Err_incomp, "text", line_no);
	  free((char*)t);
 	  return(NULL);
	}
#endif
	if (font_size) {
	    /* scale length/height of text by ratio of requested font size to actual size */
	    t->length = t->length * font_size/t->size;
	    t->height = t->height * font_size/t->size;
	    t->size = font_size;	/* and set to requested size */
	}
	if (t->size <= 0.0)
	    t->size = (float) DEFAULT_FONT_SIZE;
	more = 0;
	if (!v30_flag && n == 13)
	    more = 1;  /* in older xfig there is more if ^A wasn't found yet */
	else if (v30_flag) {	/* in 3.0 there is more if \001 wasn't found */
	    len = strlen(s);
	    if ((strcmp(&s[len-4],"\\001") == 0) &&	/* if we find '\000' */
	        !(backslash_count(s, len-5) % 2)) {	/* and not '\\000' */
		    more = 0;				/* then there are no more lines */
		    s[len-4]='\0';			/* and get rid of the '\001' */
	    } else {
		more = 1;
		s[len++]='\n';			/* put back the end of line char */
		s[len] = '\0';			/* and terminate it */
	    }
	}
	if (more) {
	/* Read in the subsequent lines of the text if there is more than one. */
	  do {
	    line_no++;		/* As is done in get_line */
	    if (v30_flag) {
		if (fgets(s_temp, BUF_SIZE, fp) == NULL)
		    break;
		len = strlen(s_temp)-1;
		if ((strncmp(&s_temp[len-4],"\\001",4) == 0) &&
		    !(backslash_count(s, len-5) % 2)) {
			n=0;			/* found the '\001', set n to stop */
			s_temp[len-4]='\0';	/* and get rid of the '\001' */
		} else {
			n=1;			/* keep going (more lines) */
		}
	    } else {
		if (fgets(buf, BUF_SIZE, fp) == NULL)
		    break;
		n = sscanf(buf, "%[^\1]%[\1]", s_temp, junk);
	    }
	    /* Safety check */
	    if (strlen(s)+1 + strlen(s_temp)+1 > (size_t)BUF_SIZE) {
	      /* Too many characters.  Ignore the rest. */
	      ignore = 1;
	    }
	    if (!ignore)
	      strcat(s, s_temp);
	  } while (n == 1);
	}
	if (v30_flag) {		/* now convert any \xxx to ascii characters */
	    if (strchr(s,'\\')) {
		int num;
		len = strlen(s);
		for (l=0,n=0; l < len; l++) {
		    if (s[l]=='\\') {
			if (l < len && s[l+1] != '\\') {
			    if (sscanf(&s[l+1],"%3o",&num)!=1) {
				put_msg("Error in parsing text string on line",line_no);
				return(NULL);
			    }
			    buf[n++]= (unsigned char) num;	/* put char in */
			    l += 3;			/* skip over digits */
			} else {
			    buf[n++] = s[++l];		/* "\\" */
			}
		    } else {
			buf[n++] = s[l];		/* ordinary character */
		    }
		}
		buf[n]='\0';		/* terminate */
		strcpy(s,buf);		/* copy back to s */
	    }
	}
	if (strlen(s) == 0) 
		(void)strcpy(s, " ");
	t->cstring = (char*)calloc((unsigned)(strlen(s)), sizeof(char));
	if (NULL == t->cstring) {
	    put_msg(Err_mem);
	    free((char*)t);
	    return(NULL);
	}
	(void)strcpy(t->cstring, s+1);

	if (!v21_flag && (t->font == 0 || t->font == DEFAULT))
		t->flags = ((t->flags != DEFAULT) ? t->flags : 0)
				| SPECIAL_TEXT;

	if (v2_flag && !v21_flag && !special_text(t)) 
		t->flags = ((t->flags != DEFAULT) ? t->flags : 0)
				| PSFONT_TEXT;

	/* keep the font number reasonable */
	if (t->font > MAXFONT(t))
		t->font = MAXFONT(t);
	fix_color(&t->color);
	return(t);
}

/* akm 28/2/95 - count consecutive backslashes backwards */
int
backslash_count(cp, start)
char cp[];
int start;
{
  int i, count = 0;

  for(i=start; i>=0; i--) {
    if (cp[i] == '\\')
	count++;
    else
	break;
  }
  return count;
}

get_line(fp)
FILE	*fp;
{
	while (1) {
	    if (NULL == fgets(buf, BUF_SIZE, fp)) {
		return(-1);
		}
	    line_no++;
	    if (*buf != '\n' && *buf != '#') 
		return(1);	/* Skip empty and comment lines */
	    }
}

skip_comment(fp)
FILE	*fp;
{
	int c;

	while ((c = fgetc(fp)) == '#') 
		skip_line(fp);
	if (c != '#') 
		ungetc(c, fp);
}

skip_line(fp)
FILE	*fp;
{
	while (fgetc(fp) != '\n') {
	    if (feof(fp)) 
		return;
	}
}

/* keep track which patterns are used (if any) */
note_pattern(fill_style)
int	 fill_style;
{
	if (fill_style >= NUMSHADES+NUMTINTS) {
	    pattern_used[fill_style-NUMSHADES-NUMTINTS] = TRUE;
	    pats_used = TRUE;
	}
}
	
init_pats_used()
{
	int i;
	pats_used = FALSE;
	for (i=0; i<NUMPATTERNS; i++)
	    pattern_used[i] = FALSE;
}
