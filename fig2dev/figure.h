/* #includes & #defines needed by *_figure.c*/

/* from xfig/fig.h */

#include <math.h>       /* for sin(), cos() etc */
#define min2(a, b)      (((a) < (b)) ? (a) : (b))
#define max2(a, b)      (((a) > (b)) ? (a) : (b))
#define signof(a)       (((a) < 0) ? -1 : 1)

/* from xfig/mode.h: */

/* rotn axis */
#define  UD_FLIP 1
#define  LR_FLIP 2
int cur_rotnangle;

/* from xfig/object.h: */

#define T_CLOSED_INTERP 3
