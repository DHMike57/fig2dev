/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1994 by Dietrich Paulus, Ruediger Bess, and Georg Stemmer
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

/* #includes & #defines needed by *_figure.c*/

#define min2(a, b)      (((a) < (b)) ? (a) : (b))
#define max2(a, b)      (((a) > (b)) ? (a) : (b))
#define signof(a)       (((a) < 0) ? -1 : 1)

/* rotn axis */
#define  UD_FLIP 1
#define  LR_FLIP 2
int cur_rotnangle;

#define T_CLOSED_INTERP 3
