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
 *	genbox : Empty box driver for fig2dev translator
 *
*/
#include <stdio.h>
#include "fig2dev.h"
#include "object.h"

void genbox_option(opt, optarg)
char opt, *optarg;
{
  	switch (opt) {

	case 's':
	case 'f':
	case 'm':
	case 'L':
		break;

 	default:
		put_msg(Err_badarg, opt, "box");
		exit(1);
	}
}

void genbox_start(objects)
F_compound	*objects;
{
	double ppi;

	if (0 == (ppi = (double)objects->nwcorner.x)) {
	    fprintf(stderr, "Resolution is zero!! default to 80 ppi\n");
	    ppi = 80.0;
	    }

	/* draw box */
        fprintf(tfp, "\\makebox[%.3fin]{\\rule{0in}{%.3fin}}\n",
		(urx-llx)*mag/ppi, (ury-lly)*mag/ppi);
	}

struct driver dev_box = {
	genbox_option,
	genbox_start,
	gendev_null,
	gendev_null,
	gendev_null,
	gendev_null,
	gendev_null,
	gendev_null,
	INCLUDE_TEXT
};
