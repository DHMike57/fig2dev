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

extern char	*malloc();
extern char	*calloc();

#define		Line_malloc(z)		z = (F_line*)malloc(LINOBJ_SIZE)
#define		Pic_malloc(z)		z = (F_pic*)malloc(PIC_SIZE)
#define		Spline_malloc(z)	z = (F_spline*)malloc(SPLOBJ_SIZE)
#define		Ellipse_malloc(z)	z = (F_ellipse*)malloc(ELLOBJ_SIZE)
#define		Arc_malloc(z)		z = (F_arc*)malloc(ARCOBJ_SIZE)
#define		Compound_malloc(z)	z = (F_compound*)malloc(COMOBJ_SIZE)
#define		Text_malloc(z)		z = (F_text*)malloc(TEXOBJ_SIZE)
#define		Point_malloc(z)		z = (F_point*)malloc(POINT_SIZE)
#define		Control_malloc(z)	z = (F_control*)malloc(CONTROL_SIZE)
#define		Arrow_malloc(z)		z = (F_arrow*)malloc(ARROW_SIZE)

extern char	Err_mem[];
extern char	Err_incomp[];
