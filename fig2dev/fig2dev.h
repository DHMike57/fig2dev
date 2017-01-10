/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1991 by Micah Beck
 * Parts Copyright (c) 1985-1988 by Supoj Sutanthavibul
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
 * Changes:
 *
 * 2015-12-01 - Declare booleans std_color_used[], arrows_used, arrow_used[].
 *		Modify for build with autoconf. (Thomas Loimer)
 *
 */

#define DEFAULT_FONT_SIZE 11

/* defined in <sys/param.h> */
#undef MIN
#undef MAX
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))

#define round(x)	((int) ((x) + ((x >= 0)? 0.5: -0.5)))

/* location for temporary files */
#define TMPDIR "/tmp"

#define	NUM_STD_COLS	32
#define	MAX_USR_COLS	512

#define NUMSHADES	21
#define NUMTINTS	20
#define NUMPATTERNS	22
#define	NUMFILLS	40	/* for convenience, NUMSHADES + NUMTINTS - 1 */

#define NUMARROWS	30	/* Synchronize with arrow_shapes[] in bound.c */

#ifndef RGB_H
#define RGB_H
typedef struct _RGB {
		unsigned short red, green, blue;
	} RGB;
#endif /* RGB_H */

/*
 * Device driver interface structure
 */
struct driver {
	void (*option)(char opt, char *optarg);	/* interpret
						 * driver-specific options */
	void (*start)();	/* output file header */
	void (*grid)(float major, float minor);	/* draw grid */
	void (*arc)();			/* object generators */
	void (*ellipse)();
	void (*line)();
	void (*spline)();
	void (*text)();
	int (*end)(void);	/* output file trailer (returns status) */
	int text_include;	/* include text length in bounding box */
#define INCLUDE_TEXT 1
#define EXCLUDE_TEXT 0
};
extern void	gendev_null(void);
extern void	gendev_nogrid(float major, float minor);

extern void	put_msg(char *fmt, ...);
extern void	unpsfont();
extern void	print_comments();

extern char	Err_badarg[];
extern char	Err_mem[];

extern char	*PSfontnames[];
extern int	PSisomap[];

extern const char	prog[];
extern char	*from, *to;
extern char	*name;
extern double	font_size;
extern double	mag, fontmag;
extern FILE	*tfp;

extern double	ppi;		/* Fig file resolution (e.g. 1200) */
extern int	llx, lly, urx, ury;
extern bool	landscape;
extern bool	center;
extern bool	multi_page;	/* multiple page option for PostScript */
extern bool	overlap;	/* overlap pages in multiple page output */
extern bool	orientspec;	/* true if the command-line args specified land or port */
extern bool	centerspec;	/* true if the command-line args specified -c or -e */
extern bool	magspec;	/* true if the command-line args specified -m */
extern bool	transspec;	/* set if the user specs. the GIF transparent color */
extern bool	paperspec;	/* true if the command-line args specified -z */
extern bool  boundingboxspec;/* true if the command-line args specified -B or -R */
extern bool	multispec;	/* true if the command-line args specified -M */
extern bool	metric;		/* true if the file contains Metric specifier */
extern char	gif_transparent[]; /* GIF transp color hex name (e.g. #ff00dd) */
extern char	papersize[];	/* paper size */
extern char	boundingbox[];	/* boundingbox */
extern float	THICK_SCALE;	/* convert line thickness from screen res. */
extern char	lang[];		/* selected output language */
extern char	*Fig_color_names[]; /* hex names for Fig colors */
extern RGB	background;	/* background (if specified by -g) */
extern bool	bgspec;		/* flag to say -g was specified */
extern float	grid_minor_spacing; /* grid minor spacing (if any) */
extern float	grid_major_spacing; /* grid major spacing (if any) */
extern char	gscom[];	/* to build up a command for ghostscript */
extern bool	psencode_header_done; /* if we have already emitted PSencode header */
extern bool	transp_header_done;   /* if we have already emitted transparent image header */
extern bool	grayonly;	/* convert colors to grayscale (-N option) */

struct paperdef
{
	char *name;		/* name for paper size */
	int width;		/* paper width in points */
	int height;		/* paper height in points */
};

/* Not used anywhere in fig2dev - but in xfig/resources.h
 *	#define NUMPAPERSIZES 29 */
extern struct paperdef paperdef[];

/* user-defined colors */
typedef	struct{
		int c,r,g,b;
	} User_color;

extern User_color	user_colors[MAX_USR_COLS];
extern int		user_col_indx[MAX_USR_COLS];
extern int		num_usr_cols;
extern bool		pats_used, pattern_used[NUMPATTERNS];
extern bool		std_color_used[NUM_STD_COLS];
extern bool		arrows_used, arrow_used[NUMARROWS];

extern void	gs_broken_pipe(int sig);
extern int	depth_filter(int obj_depth);
