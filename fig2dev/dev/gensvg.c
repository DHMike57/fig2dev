/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1999 by T. Sato
 * Parts Copyright (c) 2002 by Anthony Starks
 * Parts Copyright (c) 2002 by Martin Kroeker
 * Parts Copyright (c) 2002 by Brian V. Smith
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
 *
 * SVG driver for fig2dev
 *
 *  from fig2svg -- convert FIG 3.2 to SVG
 *
 *  Original author:  Anthony Starks (ajstarks@home.com)
 *  Created: 17 May 2000 
 *  Converted to gensvg by Brian Smith
 *  Further modified by Martin Kroeker (martin@ruby.chemie.uni-freiburg.de) 02-Dec-2002
 * 
 *  MK 04-Dec-02: partial support for the symbol font, bigger fontscale, text alignment,
 *  dashed and dotted lines, bugfix for missing % in stroke-color statement of arcs
 *  FIXME: lacks support for arrowheads; fill patterns; percent grayscale fills
 *  MK 08-Dec-02: rotated text; shades and tints of fill colors; filled circles
 *  MK 11-Dec-02: scaling;proper font/slant/weight support; changed arc code
 *  12-Dec-02: fixes by Brian Smith: scale factor, orientation, ellipse fills
 *  MK 14-Dec-02: arc code rewrite, simplified line style handling, 
 *  arrowheads on arcs and lines (FIXME: not clipped), stroke->color command
 *  is simply 'stroke'
 *  MK 15-Dec-02: catch pattern fill flags, convert to tinted fills for now
 *  MK 18-Dec-02: fill patterns; fixes by BS: arrowhead scale & position,
 *  circle by diameter
 */

#include "fig2dev.h"
#include "object.h"
#include "../../patchlevel.h"

#define PREAMBLE "<?xml version=\"1.0\" standalone=\"no\"?>\n\
<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 20000303 Stylable//EN\"\n\
             \"http://www.w3.org/TR/2000/03/WD-SVG-20000303/DTD/svg-20000303-stylable.dtd\">"

char   *ctype[] = { "none", "black", "white" };

/* arrowhead arrays */
Point   points[50];
int     npoints;
int     arrowx1, arrowy1;	/* first point of object */
int     arrowx2, arrowy2;	/* second point of object */



static F_point *p;

static unsigned int
rgbColorVal (int colorIndex)
{				/* taken from genptk.c */
    extern User_color user_colors[];
    unsigned int rgb;
    static unsigned int rgbColors[NUM_STD_COLS] = {
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
	rgb = ((user_colors[colorIndex - NUM_STD_COLS].r & 0xff) << 16)
	    | ((user_colors[colorIndex - NUM_STD_COLS].g & 0xff) << 8)
	    | (user_colors[colorIndex - NUM_STD_COLS].b & 0xff);
    return rgb;
}

static unsigned int
rgbFillVal (int colorIndex, int area_fill)
{
    extern User_color user_colors[];
    unsigned int rgb, r, g, b;
    short   tintflag = 0;
    static unsigned int rgbColors[NUM_STD_COLS] = {
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
	rgb = ((user_colors[colorIndex - NUM_STD_COLS].r & 0xff) << 16)
	    | ((user_colors[colorIndex - NUM_STD_COLS].g & 0xff) << 8)
	    | (user_colors[colorIndex - NUM_STD_COLS].b & 0xff);
    if (area_fill > 40) {
	fprintf (stderr, "Fill patterns not yet supported in SVG output\n");
	area_fill -= 20;
    }

    tintflag = 0;
    if (area_fill > 20) {
	tintflag = 1;
	area_fill -= 20;
    }
    if (colorIndex > 0 && colorIndex != 7) {
	if (tintflag) {
	    r = ((rgb & ~0xFFFF) >> 16) + (area_fill * 255 / 20);
	    if (r > 255)
		r = 255;
	    g = ((rgb & 0xFF00) >> 8) + (area_fill * 255 / 20);
	    if (g > 255)
		g = 255;
	    b = (rgb & ~0xFFFF00) + (area_fill * 255 / 20);
	    if (b > 255)
		b = 255;
	    rgb = (r << 16) + (g << 8) + b;
	}
	else
	    rgb = (((int) ((area_fill / 20.) * ((rgb & ~0xFFFF) >> 16)) << 16) +
		   ((int) ((area_fill / 20.) * ((rgb & 0xFF00) >> 8)) << 8)
		   + ((int) (area_fill / 20.) * (rgb & ~0xFFFF00)));

    }
    else {
	if (colorIndex == 0 || colorIndex == DEFAULT)
	    area_fill = 20 - area_fill;
	rgb =
	    ((area_fill * 255 / 20) << 16) + ((area_fill * 255 / 20) << 8) +
	    area_fill * 255 / 20;
    }

    return rgb;
}

static int
degrees (float angle)
{
    int     deg;

    deg = angle / 3.1415926 * 180;
    if (deg < 180)
	deg = -180 + deg;
    return deg;
}

void
gensvg_option (opt, optarg)
     char    opt;
     char   *optarg;
{
    switch (opt) {
	case 'L':		/* ignore language and magnif. */
	case 'm':
	    break;
	case 'z':
	    (void) strcpy (papersize, optarg);
	    paperspec = True;
	    break;
	default:
	    put_msg (Err_badarg, opt, "svg");
	    exit (1);
    }
}

void
gensvg_start (objects)
     F_compound *objects;
{
    struct paperdef *pd;
    int     pagewidth = -1, pageheight = -1;
    int     vw, vh;
    time_t  when;
    char    stime[80];
    int     i;

    fprintf (tfp, "%s\n", PREAMBLE);
    fprintf (tfp, "<!-- Creator: %s Version %s Patchlevel %s -->\n",
	     prog, VERSION, PATCHLEVEL);

    (void) time (&when);
    strcpy (stime, ctime (&when));
    /* remove trailing newline from date/time */
    stime[strlen (stime) - 1] = '\0';
    fprintf (tfp, "<!-- CreationDate: %s -->\n", stime);

    /* convert paper size from ppi to inches */
    for (pd = paperdef; pd->name != NULL; pd++)
	if (strcasecmp (papersize, pd->name) == 0) {
	    pagewidth = pd->width;
	    pageheight = pd->height;
	    strcpy (papersize, pd->name);	/* use the "nice" form */
	    break;
	}
    if (pagewidth < 0 || pageheight < 0) {
	(void) fprintf (stderr, "Unknown paper size `%s'\n", papersize);
	exit (1);
    }
    if (landscape) {
	vw = pagewidth;
	pagewidth = pageheight;
	pageheight = vw;
    }
    vw = (int) (pagewidth / 50 * mag * ppi);
    vh = (int) (pageheight / 50 * mag * ppi);
    fprintf (tfp,
	     "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%.1fin\" height=\"%.1fin\" viewBox=\"0 0 %d %d\">\n",
	     pagewidth / 72., pageheight / 72., vw, vh);

    if (objects->comments)
	print_comments ("<desc>", objects->comments, "</desc>");
    fprintf (tfp, "<g style=\"stroke-width:.025in; stroke:black; fill:none\">\n");
    fprintf (tfp, "<defs>\n");
    fprintf (tfp, "<pattern id=\"tile1\" x=\"0\" y=\"0\" width=\"200\" height=\"200\"\n");
    fprintf (tfp, "         patternUnits=\"userSpaceOnUse\">\n");
    for (i = -100; i < 200; i = i + 40)
	fprintf (tfp, "<path d=\"M 0 %d 200 %d\" />\n", i, (int) ceil(i+200.0 * tan (30. * M_PI / 180.)));
    fprintf (tfp, "</pattern>\n");

    fprintf (tfp, "<pattern id=\"tile2\" x=\"0\" y=\"0\" width=\"200\" height=\"200\"\n");
    fprintf (tfp, "         patternUnits=\"userSpaceOnUse\">\n");
    for (i = -100; i < 200; i = i + 40)
	fprintf (tfp, "<path d=\"M 200 %d 0 %d\" />\n", i,
		 (int) ceil (i + 200 * tan (30. * M_PI / 180.)));
    fprintf (tfp, "</pattern>\n");

    fprintf (tfp, "<pattern id=\"tile3\" x=\"0\" y=\"0\" width=\"200\" height=\"200\"\n");
    fprintf (tfp, "         patternUnits=\"userSpaceOnUse\">\n");
    for (i = -100; i < 200; i = i + 40) {
	fprintf (tfp, "<path d=\"M 0 %d 200 %d\" />\n", i,
		 (int) ceil (i + 200 * tan (30. * M_PI / 180.)));
	fprintf (tfp, "<path d=\"M 200 %d 0 %d\" />\n", i,
		 (int) ceil (i + 200 * tan (30. * M_PI / 180.)));
    }
    fprintf (tfp, "</pattern>\n");

    fprintf (tfp, "<pattern id=\"tile4\" x=\"0\" y=\"0\" width=\"200\" height=\"200\"\n");
    fprintf (tfp, "         patternUnits=\"userSpaceOnUse\">\n");
    fprintf (tfp, "<path d=\"M 100 0 200 100\" />\n");
    fprintf (tfp, "<path d=\"M 0 0 200 200\" />\n");
    fprintf (tfp, "<path d=\"M 0 100 100 200\" />\n");
    fprintf (tfp, "</pattern>\n");

    fprintf (tfp, "<pattern id=\"tile5\" x=\"0\" y=\"0\" width=\"200\" height=\"200\"\n");
    fprintf (tfp, "         patternUnits=\"userSpaceOnUse\">\n");
    fprintf (tfp, "<path d=\"M 100 0 0 100\" />\n");
    fprintf (tfp, "<path d=\"M 200 0 0 200\" />\n");
    fprintf (tfp, "<path d=\"M 200 100 100 200\" />\n");
    fprintf (tfp, "</pattern>\n");

    fprintf (tfp, "<pattern id=\"tile6\" x=\"0\" y=\"0\" width=\"200\" height=\"200\"\n");
    fprintf (tfp, "         patternUnits=\"userSpaceOnUse\">\n");
    fprintf (tfp, "<path d=\"M 100 0 200 100\" />\n");
    fprintf (tfp, "<path d=\"M 0 0 200 200\" />\n");
    fprintf (tfp, "<path d=\"M 0 100 100 200\" />\n");
    fprintf (tfp, "<path d=\"M 100 0 0 100\" />\n");
    fprintf (tfp, "<path d=\"M 200 0 0 200\" />\n");
    fprintf (tfp, "<path d=\"M 200 100 100 200\" />\n");
    fprintf (tfp, "</pattern>\n");

    fprintf (tfp, "<pattern id=\"tile7\" x=\"0\" y=\"0\" width=\"200\" height=\"200\"\n");
    fprintf (tfp, "         patternUnits=\"userSpaceOnUse\">\n");
    fprintf (tfp, "<path d=\"M 0 0 0 50\" />\n");
    fprintf (tfp, "<path d=\"M 0 50 200 50\" />\n");
    fprintf (tfp, "<path d=\"M 100 50 100 150\" />\n");
    fprintf (tfp, "<path d=\"M 0 150 200 150\" />\n");
    fprintf (tfp, "<path d=\"M 0 150 0 200\" />\n");
    fprintf (tfp, "</pattern>\n");

    fprintf (tfp, "<pattern id=\"tile8\" x=\"0\" y=\"0\" width=\"200\" height=\"200\"\n");
    fprintf (tfp, "         patternUnits=\"userSpaceOnUse\">\n");
    fprintf (tfp, "<path d=\"M 0 0 50 0\" />\n");
    fprintf (tfp, "<path d=\"M 50 0 50 200\" />\n");
    fprintf (tfp, "<path d=\"M 50 100 150 100\" />\n");
    fprintf (tfp, "<path d=\"M 150 0 150 200\" />\n");
    fprintf (tfp, "<path d=\"M 150 0 200 0\" />\n");
    fprintf (tfp, "</pattern>\n");

    fprintf (tfp, "<pattern id=\"tile9\" x=\"0\" y=\"0\" width=\"200\" height=\"200\"\n");
    fprintf (tfp, "         patternUnits=\"userSpaceOnUse\">\n");
    fprintf (tfp, "<path d=\"M 0 50 200 50\" />\n");
    fprintf (tfp, "<path d=\"M 0 150 200 150\" />\n");
    fprintf (tfp, "</pattern>\n");

    fprintf (tfp,
	     "<pattern id=\"tile10\" x=\"0\" y=\"0\" width=\"200\" height=\"200\"\n");
    fprintf (tfp, "         patternUnits=\"userSpaceOnUse\">\n");
    fprintf (tfp, "<path d=\"M 50 0 50 200\" />\n");
    fprintf (tfp, "<path d=\"M 150 0 150 200\" />\n");
    fprintf (tfp, "</pattern>\n");

    fprintf (tfp,
	     "<pattern id=\"tile11\" x=\"0\" y=\"0\" width=\"200\" height=\"200\"\n");
    fprintf (tfp, "         patternUnits=\"userSpaceOnUse\">\n");
    fprintf (tfp, "<path d=\"M 0 50 200 50\" />\n");
    fprintf (tfp, "<path d=\"M 0 150 200 150\" />\n");
    fprintf (tfp, "<path d=\"M 50 0 50 200\" />\n");
    fprintf (tfp, "<path d=\"M 150 0 150 200\" />\n");
    fprintf (tfp, "</pattern>\n");

    fprintf (tfp,
	     "<pattern id=\"tile12\" x=\"0\" y=\"0\" width=\"200\" height=\"200\"\n");
    fprintf (tfp, "         patternUnits=\"userSpaceOnUse\">\n");
    fprintf (tfp, "<path d=\"M 0 0 25 50\" />\n");
    fprintf (tfp, "<path d=\"M 0 50 200 50\" />\n");
    fprintf (tfp, "<path d=\"M 100 50 125 150\" />\n");
    fprintf (tfp, "<path d=\"M 0 150 200 150\" />\n");
    fprintf (tfp, "<path d=\"M 0 150 25 200\" />\n");
    fprintf (tfp, "</pattern>\n");

    fprintf (tfp,
	     "<pattern id=\"tile13\" x=\"0\" y=\"0\" width=\"200\" height=\"200\"\n");
    fprintf (tfp, "         patternUnits=\"userSpaceOnUse\">\n");
    fprintf (tfp, "<path d=\"M 200 0 175 50\" />\n");
    fprintf (tfp, "<path d=\"M 0 50 200 50\" />\n");
    fprintf (tfp, "<path d=\"M 100 50 75 150\" />\n");
    fprintf (tfp, "<path d=\"M 0 150 200 150\" />\n");
    fprintf (tfp, "<path d=\"M 200 150 175 200\" />\n");
    fprintf (tfp, "</pattern>\n");

    fprintf (tfp,
	     "<pattern id=\"tile14\" x=\"0\" y=\"0\" width=\"200\" height=\"200\"\n");
    fprintf (tfp, "         patternUnits=\"userSpaceOnUse\">\n");
    fprintf (tfp, "<path d=\"M 0 0 50 25\" />\n");
    fprintf (tfp, "<path d=\"M 50 0 50 200\" />\n");
    fprintf (tfp, "<path d=\"M 50 100 150 125\" />\n");
    fprintf (tfp, "<path d=\"M 150 0 150 200\" />\n");
    fprintf (tfp, "<path d=\"M 150 0 200 25\" />\n");
    fprintf (tfp, "</pattern>\n");

    fprintf (tfp,
	     "<pattern id=\"tile15\" x=\"0\" y=\"0\" width=\"200\" height=\"200\"\n");
    fprintf (tfp, "         patternUnits=\"userSpaceOnUse\">\n");
    fprintf (tfp, "<path d=\"M 0 25 50 0\" />\n");
    fprintf (tfp, "<path d=\"M 50 0 50 200\" />\n");
    fprintf (tfp, "<path d=\"M 50 125 150 100\" />\n");
    fprintf (tfp, "<path d=\"M 150 0 150 200\" />\n");
    fprintf (tfp, "<path d=\"M 150 25 200 0\" />\n");
    fprintf (tfp, "</pattern>\n");

    fprintf (tfp,
	     "<pattern id=\"tile16\" x=\"0\" y=\"0\" width=\"200\" height=\"200\"\n");
    fprintf (tfp, "         patternUnits=\"userSpaceOnUse\">\n");
    fprintf (tfp, "<path d=\"M 0 50 A 50 50 0 1 0 100 50\" />\n");
    fprintf (tfp, "<path d=\"M 100 50 A 50 50 0 1 0 200 50\" />\n");
    fprintf (tfp, "<path d=\"M 50 100 A 50 50 0 1 0 150 100\" />\n");
    fprintf (tfp, "<path d=\"M 0 150 A 50 50 0 0 0 50 100\" />\n");
    fprintf (tfp, "<path d=\"M 150 100 A 50 50 0 1 0 200 50\" />\n");
    fprintf (tfp, "<path d=\"M 50 0 A 50 50 0 1 0 150 0\" />\n");
    fprintf (tfp, "<path d=\"M 150 0 A 50 50 0 0 0 200 50\" />\n");
    fprintf (tfp, "<path d=\"M 0 50 A 50 50 0 0 0 50 0\" />\n");
    fprintf (tfp, "<path d=\"M 0 150 A 50 50 0 1 0 100 150\" />\n");
    fprintf (tfp, "<path d=\"M 100 150 A 50 50 0 1 0 200 150\" />\n");
    fprintf (tfp, "</pattern>\n");

    fprintf (tfp,
	     "<pattern id=\"tile17\" x=\"0\" y=\"0\" width=\"100\" height=\"100\"\n");
    fprintf (tfp, "         patternUnits=\"userSpaceOnUse\">\n");
    fprintf (tfp, "<g transform=\"scale(0.5)\" >\n");
    fprintf (tfp, "<path d=\"M 0 50 A 50 50 0 1 0 100 50\" />\n");
    fprintf (tfp, "<path d=\"M 100 50 A 50 50 0 1 0 200 50\" />\n");
    fprintf (tfp, "<path d=\"M 50 100 A 50 50 0 1 0 150 100\" />\n");
    fprintf (tfp, "<path d=\"M 0 150 A 50 50 0 0 0 50 100\" />\n");
    fprintf (tfp, "<path d=\"M 150 100 A 50 50 0 1 0 200 50\" />\n");
    fprintf (tfp, "<path d=\"M 50 0 A 50 50 0 1 0 150 0\" />\n");
    fprintf (tfp, "<path d=\"M 150 0 A 50 50 0 0 0 200 50\" />\n");
    fprintf (tfp, "<path d=\"M 0 50 A 50 50 0 0 0 50 0\" />\n");
    fprintf (tfp, "<path d=\"M 0 150 A 50 50 0 1 0 100 150\" />\n");
    fprintf (tfp, "<path d=\"M 100 150 A 50 50 0 1 0 200 150\" />\n");
    fprintf (tfp, "</g>\n");
    fprintf (tfp, "</pattern>\n");

    fprintf (tfp,
	     "<pattern id=\"tile18\" x=\"0\" y=\"0\" width=\"200\" height=\"200\"\n");
    fprintf (tfp, "         patternUnits=\"userSpaceOnUse\">\n");
    fprintf (tfp, "<circle cx=\"100\" cy=\"100\" r=\"100\" />\n");
    fprintf (tfp, "</pattern>\n");

    fprintf (tfp,
	     "<pattern id=\"tile19\" x=\"0\" y=\"0\" width=\"200\" height=\"200\"\n");
    fprintf (tfp, "         patternUnits=\"userSpaceOnUse\">\n");
    fprintf (tfp, "<path d=\"M 0 50 45 0 105 0 140 50 200 50 \" />\n");
    fprintf (tfp, "<path d=\"M 0 50 45 100 105 100 140 50 200 50\" />\n");
    fprintf (tfp, "<path d=\"M 0 150 45 100 105 100 140 150 200 150\" />\n");
    fprintf (tfp, "<path d=\"M 0 150 45 200 105 200 140 150 200 150\" />\n");
    fprintf (tfp, "</pattern>\n");

    fprintf (tfp,
	     "<pattern id=\"tile20\" x=\"0\" y=\"0\" width=\"200\" height=\"200\"\n");
    fprintf (tfp, "         patternUnits=\"userSpaceOnUse\">\n");
    fprintf (tfp, "<path d=\"M 0 70 65 0 140 0 200 70 \" />\n");
    fprintf (tfp, "<path d=\"M 0 70 0 130 65 200 140 200 200 130 200 70\" />\n");
    fprintf (tfp, "</pattern>\n");

    fprintf (tfp,
	     "<pattern id=\"tile21\" x=\"0\" y=\"0\" width=\"200\" height=\"200\"\n");
    fprintf (tfp, "         patternUnits=\"userSpaceOnUse\">\n");
    fprintf (tfp, "<path d=\"M 50 0 75 25 100 0 M 150 0 175 25 200 0\" />\n");
    fprintf (tfp, "<path d=\"M 0 50 25 25 75 75 125 25 175 75 200 50\" />\n");
    fprintf (tfp, "<path d=\"M 0 100 25 75 75 125 125 75 175 125 200 100\" />\n");
    fprintf (tfp, "<path d=\"M 0 150 25 125 75 175 125 125 175 175 200 150\" />\n");
    fprintf (tfp, "<path d=\"M 0 200 25 175 75 225 125 175 175 225 200 200\" />\n");
    fprintf (tfp, "</pattern>\n");

    fprintf (tfp,
	     "<pattern id=\"tile22\" x=\"0\" y=\"0\" width=\"200\" height=\"200\"\n");
    fprintf (tfp, "         patternUnits=\"userSpaceOnUse\">\n");
    fprintf (tfp, "<path d=\"M 0 50 25 75 0 100 M 0 150 25 175 0 200\" />\n");
    fprintf (tfp, "<path d=\"M 50 0 25 25 75 75 25 125 75 175 50 200\" />\n");
    fprintf (tfp, "<path d=\"M 100 0 75 25 125 75 75 125 125 175 100 200\" />\n");
    fprintf (tfp, "<path d=\"M 150 0 125 25 175 75 125 125 175 175 150 200\" />\n");
    fprintf (tfp, "<path d=\"M 200 0 175 25 225 75 175 125 225 175 200 200\" />\n");
    fprintf (tfp, "</pattern>\n");

    fprintf (tfp, "</defs>\n");

}

gensvg_end ()
{
    fprintf (tfp, "</g>\n</svg>\n");
    return 0;
}

void
gensvg_line (l)
     F_line *l;
{
    const char *linestyle[5] =
	{ "50 50", "10 10 10", "50 10 10", "50 10 10 10 10", "50 10 10 10 10 10 10" };
    int     i;
    int     c1x, c1y, c2x, c2y;	/* dummy for bounding box */
    int     nboundpts;

    fprintf (tfp, "<!-- Line -->\n");
    print_comments ("<!-- ", l->comments, " -->");
    fprintf (tfp, "<path d=\"M ");
    for (p = l->points; p; p = p->next) {
	fprintf (tfp, "%d,%d\n", (int) (p->x * mag), (int) (p->y * mag));
	arrowx1 = arrowx2;
	arrowy1 = arrowy2;
	arrowx2 = p->x;
	arrowy2 = p->y;
    }

    fprintf (tfp, "\" style=\"stroke:#%6.6x;stroke-width:%d;\n",
	     rgbColorVal (l->pen_color), (int) ceil (l->thickness * mag));

    if (l->style > 0)
	fprintf (tfp, "stroke-dasharray:%s;\n", linestyle[l->style - 1]);

    if (l->fill_style != -1 && l->fill_style < 41)
	fprintf (tfp, "fill:#%6.6x;\n", rgbFillVal (l->fill_color, l->fill_style));
    if (l->fill_style > 40)
	fprintf (tfp, "fill:url(#tile%d);\n", l->fill_style - 40);
    fprintf (tfp, "\"/>\n");

    if (l->for_arrow) {
	calc_arrow (arrowx1, arrowy1, arrowx2, arrowy2,
		    &c1x, &c1y, &c2x, &c2y, l->for_arrow, points, &npoints, &nboundpts);

	fprintf (tfp, "<!-- Arrowhead on endpoint -->\n");
	fprintf (tfp, "<path d=\"M ");
	for (i = 0; i < npoints; i++) {
	    fprintf (tfp, "%d %d\n", (int) (points[i].x * mag),
		     (int) (points[i].y * mag));
	}
	if (l->for_arrow->type > 0)
	    fprintf (tfp, "Z\n");
	fprintf (tfp, "\" style=\"stroke:#%6.6x;stroke-width:%d;\n",
		 rgbColorVal (l->pen_color), (int) ceil (l->for_arrow->thickness * mag));
	if (l->for_arrow->type > 0) {
	    if (l->for_arrow->style == 0)
		fprintf (tfp, "fill:white;\"/>\n");
	    else
		fprintf (tfp, "fill:#%6.6x;\"/>\n", rgbColorVal (l->pen_color));
	}
	else
	    fprintf (tfp, "\"/>\n");
    }

    if (l->back_arrow) {
	p = l->points;
	arrowx2 = p->x;
	arrowy2 = p->y;
	p = p->next;
	arrowx1 = p->x;
	arrowy1 = p->y;
	calc_arrow (arrowx1, arrowy1, arrowx2, arrowy2,
		    &c1x, &c1y, &c2x, &c2y, l->back_arrow, points, &npoints, &nboundpts);

	fprintf (tfp, "<!-- Arrowhead on startpoint -->\n");
	fprintf (tfp, "<path d=\"M ");
	for (i = 0; i < npoints; i++) {
	    fprintf (tfp, "%d %d\n", (int) (points[i].x * mag),
		     (int) (points[i].y * mag));
	}
	if (l->back_arrow->type > 0)
	    fprintf (tfp, "Z\n");
	fprintf (tfp, "\" style=\"stroke:#%6.6x;stroke-width:%d;\n",
		 rgbColorVal (l->pen_color), (int) ceil (l->back_arrow->thickness * mag));
	if (l->back_arrow->type > 0) {
	    if (l->back_arrow->style == 0)
		fprintf (tfp, "fill:white;\"/>\n");
	    else
		fprintf (tfp, "fill:#%6.6x;\"/>\n", rgbColorVal (l->pen_color));
	}
	else
	    fprintf (tfp, "\"/>\n");
    }
}

void
gensvg_spline (s)
     F_spline *s;
{
    fprintf (tfp, "<!-- Spline -->\n");
    print_comments ("<!-- ", s->comments, " -->");

    fprintf (tfp, "<path style=\"stroke:#%6.6x;stroke-width:%d\" d=\"",
	     rgbColorVal (s->pen_color), (int) ceil (s->thickness * mag));
    fprintf (tfp, "M %d,%d \n C", (int) s->points->x * mag, (int) s->points->y * mag);
    for (p = s->points++; p; p = p->next) {
	fprintf (tfp, "%d,%d\n", (int) p->x * mag, (int) p->y * mag);
    }
    fprintf (tfp, "\"/>\n");
}

void
gensvg_arc (a)
     F_arc  *a;
{
    int     radius;
    double  angle1, angle2, angle, dx, dy;
    int     i, c1x, c1y, c2x, c2y, nboundpts;

    fprintf (tfp, "<!-- Arc -->\n");
    print_comments ("<!-- ", a->comments, " -->");

    dx = a->point[0].x - a->center.x;
    dy = a->point[0].y - a->center.y;
    radius = sqrt (dx * dx + dy * dy);
    if (a->center.x == a->point[0].x)
	angle1 = (a->point[0].y - a->center.y > 0 ? 90.0 : -90.0);
    else
	angle1 =
	    atan2 (a->point[0].y - a->center.y,
		   a->point[0].x - a->center.x) * 180. / M_PI;
    if (a->center.x == a->point[2].x)
	angle2 = (a->point[2].y - a->center.y > 0 ? 90.0 : -90.0);
    else
	angle2 =
	    atan2 (a->point[2].y - a->center.y,
		   a->point[2].x - a->center.x) * 180. / M_PI;
    angle = angle2 - angle1;

    fprintf (tfp, "<path style=\"stroke:#%6.6x;stroke-width:%d\" d=\"M %d,%d A %d %d % d % d % d % d % d \" />\n",
	     rgbColorVal (a->pen_color), (int) ceil (a->thickness * mag),
	     (int) (a->point[0].x * mag), (int) (a->point[0].y * mag),
	     (int) (radius * mag), (int) (radius * mag),
	     0,
	     (((fabs (angle) > 180 && a->direction == 0) || angle < 0
	       && a->direction == 0) ? 1 : 0), (((angle > 0 && a->direction == 0)
						 || a->direction == 0) ? 1 : 0),
	     (int) (a->point[2].x * mag), (int) (a->point[2].y * mag));

    if (a->for_arrow) {
	arrowx1 = a->point[2].x;
	arrowy1 = a->point[2].y;
	compute_arcarrow_angle (a->center.x, a->center.y,
				(double) arrowx1, (double) arrowy1,
				a->direction, a->for_arrow, &arrowx2, &arrowy2);

	calc_arrow (arrowx2, arrowy2, arrowx1, arrowy1,
		    &c1x, &c1y, &c2x, &c2y, a->for_arrow, points, &npoints, &nboundpts);

	fprintf (tfp, "<!-- Arrowhead on endpoint -->\n");
	fprintf (tfp, "<path d=\"M ");
	for (i = 0; i < npoints; i++) {
	    fprintf (tfp, "%d %d\n", (int) (points[i].x * mag),
		     (int) (points[i].y * mag));
	}
	if (a->for_arrow->type > 0)
	    fprintf (tfp, "Z\n");
	fprintf (tfp, "\" style=\"stroke:#%6.6x;stroke-width:%d;\n",
		 rgbColorVal (a->pen_color), (int) ceil (a->for_arrow->thickness * mag));
	if (a->for_arrow->type > 0) {
	    if (a->for_arrow->style == 0)
		fprintf (tfp, "fill:white;\"/>\n");
	    else
		fprintf (tfp, "fill:#%6.6x;\"/>\n", rgbColorVal (a->pen_color));
	}
	else
	    fprintf (tfp, "\"/>\n");
    }

    if (a->back_arrow) {
	arrowx1 = a->point[0].x;
	arrowy1 = a->point[0].y;
	compute_arcarrow_angle (a->center.x, a->center.y,
				(double) arrowx1, (double) arrowy1,
				a->direction ^ 1, a->back_arrow, &arrowx2, &arrowy2);
	calc_arrow (arrowx2, arrowy2, arrowx1, arrowy1,
		    &c1x, &c1y, &c2x, &c2y, a->back_arrow, points, &npoints, &nboundpts);

	fprintf (tfp, "<!-- Arrowhead on startpoint -->\n");
	fprintf (tfp, "<path d=\"M ");
	for (i = 0; i < npoints; i++) {
	    fprintf (tfp, "%d %d\n", (int) (points[i].x * mag),
		     (int) (points[i].y * mag));
	}
	if (a->back_arrow->type > 0)
	    fprintf (tfp, "Z\n");
	fprintf (tfp, "\" style=\"stroke:#%6.6x;stroke-width:%d;\n",
		 rgbColorVal (a->pen_color), (int) ceil (a->back_arrow->thickness * mag));
	if (a->back_arrow->type > 0) {
	    if (a->back_arrow->style == 0)
		fprintf (tfp, "fill:white;\"/>\n");
	    else
		fprintf (tfp, "fill:#%6.6x;\"/>\n", rgbColorVal (a->pen_color));
	}
	else
	    fprintf (tfp, "\"/>\n");
    }


}

void
gensvg_ellipse (e)
     F_ellipse *e;
{
    if (e->type == T_CIRCLE_BY_RAD || e->type == T_CIRCLE_BY_DIA) {
	fprintf (tfp, "<!-- Circle -->\n");
	print_comments ("<!-- ", e->comments, " -->");
	fprintf (tfp, "<circle cx=\"%d\" cy=\"%d\" r=\"%d\" \n",
		 (int) (e->center.x * mag), (int) (e->center.y * mag),
		 (int) (fabs (e->end.x - e->start.x) * mag));
	if (e->fill_style == -1)
	    fprintf (tfp, " style=\"stroke:#%6.6x;stroke-width:%d\" />\n",
		     rgbColorVal (e->pen_color), (int) ceil (e->thickness * mag));
	else
	    fprintf (tfp, " style=\"stroke:#%6.6x;stroke-width:%d;fill:#%6.6x\" />\n",
		     rgbColorVal (e->pen_color), (int) ceil (e->thickness * mag),
		     rgbFillVal (e->fill_color, e->fill_style));

    }
    else {
	fprintf (tfp, "<!-- Ellipse -->\n");
	print_comments ("<!-- ", e->comments, " -->");
	fprintf (tfp, "<ellipse cx=\"%d\" cy=\"%d\" rx=\"%d\" ry=\"%d\" ",
		 (int) (e->center.x * mag), (int) (e->center.y * mag),
		 (int) (e->radiuses.x * mag), (int) (e->radiuses.y * mag));
	if (e->fill_style != -1)
	    fprintf (tfp, "style=\"fill:%s\"/>\n", ctype[e->type]);
	else
	    fprintf (tfp, "/>\n");
    }
}

void
gensvg_text (t)
     F_text *t;
{
    unsigned char *cp;
    unsigned int ch;
    const char *anchor[3] = { "start", "middle", "end" };
    static const char *family[9] = { "Times", "AvantGarde",
	"Bookman", "Courier", "Helvetica", "Helvetica Narrow",
	"New Century Schoolbook", "Palatino", "Times"
    };

    fprintf (tfp, "<!-- Text -->\n");
    print_comments ("<!-- ", t->comments, " -->");

    if (t->angle != 0) {
	fprintf (tfp, "<g transform=\"translate(%d,%d) rotate(%d)\" >\n",
		 (int) (t->base_x * mag), (int) (t->base_y * mag), degrees (t->angle));
	fprintf (tfp, "<text x=\"0\" y=\"0\" fill=\"#%6.6x\"  font-family=\"%s\" 
		 font-style=\"%s\" font-weight=\"%s\" font-size=\"%d\" text-anchor=\"%s\" >\n",
		 rgbColorVal (t->color), family[(int) ceil ((t->font + 1) / 4)],
		 (t->font % 2 == 0 ? "normal" : "italic"),
		 (t->font % 4 < 2 ? "normal" : "bold"), (int) (ceil (t->size * 12 * mag)),
		 anchor[t->type]);
    }
    else
	fprintf (tfp, "<text x=\"%d\" y=\"%d\" fill=\"#%6.6x\"  font-family=\"%s\" 
		 font-style=\"%s\" font-weight=\"%s\" font-size=\"%d\" text-anchor=\"%s\" >\n",
		 (int) (t->base_x * mag), (int) (t->base_y * mag), rgbColorVal (t->color),
		 family[(int) ceil ((t->font + 1) / 4)],
		 (t->font % 2 == 0 ? "normal" : "italic"),
		 (t->font % 4 < 2 ? "normal" : "bold"), (int) (ceil (t->size * 12 * mag)),
		 anchor[t->type]);

    if (t->font == 32) {
	for (cp = (unsigned char *) t->cstring; *cp; cp++) {
	    ch = *cp;
	    ch += 848;		/* transform from the X11 symbol font [5~to unicode greek */
	    switch (ch) {	/* which uses a different sort order */
		case 915:
		/*C*/ case 947:	/*c */
		    ch += 20;
		    break;
		case 918:
		case 950:	/*f */
		    ch += 16;
		    break;
		case 919:
		case 951:	/*g */
		    ch -= 4;
		    break;
		case 920:
		case 952:	/*h */
		    ch -= 1;
		    break;
		case 922:	/*J is vartheta */
		    ch = 977;
		    break;
		case 954:	/*j is varphi */
		    ch = 981;
		    break;
		case 923:
		case 924:
		case 925:
		case 955:
		case 956:
		case 957:
		    ch -= 1;
		    break;
		case 926:
		    /*N*/ ch = 78;
		    break;
		case 958:	/*n */
		    ch = 118;
		    break;
		case 929:
		    /*Q*/ ch = 920;
		    break;
		case 961:	/*p */
		    ch = 952;
		    break;
		case 962:
		    ch -= 1;
		    break;
		case 966:	/*v */
		    ch = 982;
		    break;
		case 967:	/*w */
		    ch = 969;
		    break;
		case 968:
		    ch = 958;
		    break;
		case 969:
		    ch -= 1;
		    break;
		case 970:
		    ch = 950;
		    break;
		case 930:
		    /*R*/ ch -= 1;
		    break;
		case 934:
		    /*V*/ ch = 962;
		    break;
		case 935:
		    /*W*/ ch += 2;
		    break;
		case 936:
		    /*X*/ ch = 926;
		    break;
		case 937:
		    /*Y*/ ch -= 1;
		    break;
		case 938:
		    ch = 90;
		    break;
		case 775:	/* bullet */
		    ch = 8226;
		    break;
		default:
		    break;
	    }
	    fprintf (tfp, "&#%04d;", ch);
	}
    }
    else {
	for (cp = (unsigned char *) t->cstring; *cp; cp++) {
	    ch = *cp;
	    if (ch < 128)
		fputc (ch, tfp);
	    else
		fprintf (tfp, "&#%d;", ch);
	}
    }
    fprintf (tfp, "</text>\n");
    if (t->angle != 0)
	fprintf (tfp, "</g>");
}



/* driver defs */

struct driver dev_svg = {
    gensvg_option,
    gensvg_start,
    gendev_null,
    gensvg_arc,
    gensvg_ellipse,
    gensvg_line,
    gensvg_spline,
    gensvg_text,
    gensvg_end,
    INCLUDE_TEXT
};
