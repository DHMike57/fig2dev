/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1989-1999 by Brian V. Smith
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

#include "fig2dev.h"
#include "object.h"

/* return codes:  1 : success
		  0 : failure
*/

int
read_eps(file, filetype, pic, llx, lly)
    FILE	   *file;
    int		    filetype;
    F_pic	   *pic;
    int		   *llx, *lly;
{
	char	    buf[512];
	double	    fllx, flly, furx, fury;

	pic->bit_size.x = pic->bit_size.y = 0;
	pic->subtype = P_EPS;

	/* give some initial values for bounding in case none is found */
	*llx = 0;
	*lly = 0;
	pic->bit_size.x = 10;
	pic->bit_size.y = 10;

	while (fgets(buf, 512, file) != NULL) {
	    char *c;

	    if (!strncmp(buf, "%%BoundingBox:", 14)) {
		c=buf+14;
		/* skip past white space */
		while (*c == ' ' || *c == '\t') 
		    c++;
		if (strncmp(c,"(atend)",7)) {	/* make sure not an (atend) */
		    if (sscanf(c, "%lf %lf %lf %lf",
				&fllx, &flly, &furx, &fury) < 4) {
			fprintf(stderr,"Bad EPS bitmap file: %s\n", pic->file);
			return 0;
		    }
		    *llx = floor(fllx);
		    *lly = floor(flly);
		    pic->bit_size.x = (furx-fllx);
		    pic->bit_size.y = (fury-flly);
		    break;
		}
	    }
	}
	fprintf(tfp, "%% Begin Imported EPS File: %s\n", pic->file);
	fprintf(tfp, "%%%%BeginDocument: %s\n", pic->file);
	fprintf(tfp, "%%\n");
	return 1;
}
