/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1991 by Micah Beck
 * Parts Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-1999 by Brian V. Smith
 *
 * Any party obtaining a copy of these files is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and
 * documentation files (the "Software"), including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons who receive
 * copies from any such party to do so, with the only requirement being
 * that this copyright notice remain intact.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include "patchlevel.h"
#include <math.h>
#include "pi.h"

/* location for temporary files */
#define TMPDIR "/tmp"

typedef char Boolean;
#define	NO	2
#define	False	0
#define	True	1

#define DEFAULT_FONT_SIZE 11

#ifdef USE_INLINE
#define INLINE inline
#else
#define INLINE
#endif /* USE_INLINE */

/* include ctype.h for isascii() and isxdigit() macros */
#include <ctype.h>

#if defined(SYSV) || defined(SVR4)
#include <string.h>
#else
#include <strings.h>
#ifndef strchr
extern char *strchr();
#endif
#if !defined(NeXT) && !defined(SYSV) && !defined(SVR4)
#define	strchr	index
#define	strrchr	rindex
#endif
#endif

#if defined(hpux) || defined(SYSV) || defined(SVR4)
#define bzero(s,n) memset((s),'\0',(n))
#endif

extern	double	atof();

#define round(x)	((int) ((x) + ((x >= 0)? 0.5: -0.5)))

#define	NUM_STD_COLS	32
#define	MAX_USR_COLS	512

#define NUMSHADES	21
#define NUMTINTS	20
#define NUMPATTERNS     22

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
 	void (*option)();	/* interpret driver-specific options */
  	void (*start)();	/* output file header */
	void (*arc)();		/* object generators */
	void (*ellipse)();
	void (*line)();
	void (*spline)();
	void (*text)();
	int (*end)();		/* output file trailer (returns status) */
  	int text_include;	/* include text length in bounding box */
#define INCLUDE_TEXT 1
#define EXCLUDE_TEXT 0
};

extern char Err_badarg[];
extern char Err_mem[];

extern char *PSfontnames[];

extern int   PSisomap[];

extern char	*prog, *from, *to;
extern char	*name;
extern int	font_size;
extern double	mag;
extern FILE	*tfp;

extern int	ppi;		/* Fig file resolution (e.g. 1200) */
extern int	llx, lly, urx, ury;
extern Boolean	landscape;
extern Boolean	center;
extern Boolean	multi_page;	/* multiple page option for PostScript */
extern Boolean	orientspec;	/* true if the command-line args specified land or port */
extern Boolean	centerspec;	/* true if the command-line args specified -c or -e */
extern Boolean	magspec;	/* true if the command-line args specified -m */
extern Boolean	transspec;	/* set if the user specs. the GIF transparent color */
extern Boolean	paperspec;	/* true if the command-line args specified -z */
extern Boolean	multispec;	/* true if the command-line args specified -M */
extern Boolean	metric;		/* true if the file contains Metric specifier */
extern char	gif_transparent[]; /* GIF transp color hex name (e.g. #ff00dd) */
extern char	papersize[];	/* paper size */
extern float	THICK_SCALE;	/* convert line thickness from screen res. */
extern char	lang[];		/* selected output language */
extern char	*Fig_color_names[]; /* hex names for Fig colors */
extern RGB	background;	/* background (if specified by -g) */
extern Boolean	bgspec;		/* flag to say -g was specified */
extern char	gscom[];	/* to build up a command for ghostscript */

struct paperdef
{
    char *name;			/* name for paper size */
    int width;			/* paper width in points */
    int height;			/* paper height in points */
};

#define NUMPAPERSIZES 29
extern struct paperdef paperdef[];

/* user-defined colors */
typedef		struct{
			int c,r,g,b;
			}
		User_color;

extern User_color	user_colors[MAX_USR_COLS];
extern int		user_col_indx[MAX_USR_COLS];
extern int		num_usr_cols;
extern Boolean		pats_used, pattern_used[NUMPATTERNS];

extern void	gendev_null();
extern void	gs_broken_pipe();

/* for GIF files */
#define	MAXCOLORMAPSIZE 256

struct Cmap {
	unsigned short red, green, blue;
	unsigned long pixel;
};

/* define PATH_MAX if not already defined */
/* taken from the X11R5 server/os/osfonts.c file */
#ifndef X_NOT_POSIX
#ifdef _POSIX_SOURCE
#include <limits.h>
#else
#if !defined(sun) || defined(sparc)
#define _POSIX_SOURCE
#include <limits.h>
#undef _POSIX_SOURCE
#endif /* !defined(sun) || defined(sparc) */
#endif /* _POSIX_SOURCE */
#endif /* X_NOT_POSIX */

#ifndef PATH_MAX
#include <sys/param.h>
#ifdef MAXPATHLEN
#define PATH_MAX MAXPATHLEN
#else
#define PATH_MAX 1024
#endif /* MAXPATHLEN */
#endif /* PATH_MAX */

#ifndef __NetBSD__
extern int		sys_nerr, errno;
#endif

#if ( !(defined(BSD) && (BSD >= 199306)) && !defined(__NetBSD__) && \
	!defined(__GNU_LIBRARY__) && !defined(__FreeBSD__) && !defined(__GLIBC__))
	    extern char *sys_errlist[];
#endif

typedef struct _point
{
    int x,y;
} Point;

