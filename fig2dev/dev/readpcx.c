/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1992 by Brian Boyter
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

#include <unistd.h>
#include "fig2dev.h"
#include "object.h"
#include "pcx.h"

int	_read_pcx();
void	readpcxhead();

/* return codes:  1 : success
		  0 : invalid file
*/

int
read_pcx(file,filetype,pic,llx,lly)
    FILE	   *file;
    int		    filetype;
    F_pic	   *pic;
    int		   *llx, *lly;
{
	*llx = *lly = 0;
	return _read_pcx(file, pic);
}

/* _read_pcx() is called from read_pcx(), read_gif(), read_ppm(), read_tif(), and read_epsf().
*/

void pcx_decode();

_read_pcx(pcxfile, pic)
    FILE	*pcxfile;
    F_pic	*pic;
{
    pcxheadr	        pcxhead;	/* PCX header */
    unsigned short	wid;		/* Width of image */
    unsigned short	ht;		/* Height of image */
    unsigned char	*buffer;	/* current input line */
    int			i, bufsize;

    fprintf(tfp, "%% Begin Imported PCX File: %s\n\n", pic->file);
    pic->subtype = P_PCX;

    /* Read the PCX image file header information */
    (void) readpcxhead(&pcxhead, pcxfile);

    /* Check for FILE stream error */
    if (ferror(pcxfile)) {
	return 0;
    }

    /* Check the identification byte value */
    if (pcxhead.id != 0x0A) {
	return 0;
    }

    /* copy the EGA palette now in case there is no VGA palette later in the file */
    for (i=0; i<16; i++) {
	pic->cmap[0][i] = (unsigned short) pcxhead.egapal[i*3];
	pic->cmap[1][i] = (unsigned short) pcxhead.egapal[i*3+1];
	pic->cmap[2][i] = (unsigned short) pcxhead.egapal[i*3+2];
    }
    pic->numcols = 16;

    /* Calculate size of image in pixels and scan lines */
    wid = pcxhead.xmax - pcxhead.xmin + 1;
    ht = pcxhead.ymax - pcxhead.ymin + 1;

    /* put in the width/height now in case there is some other failure later */
    pic->bit_size.x = wid;
    pic->bit_size.y = ht;

    /* allocate space for the image (allocate a little more than needed to be safe) */
    bufsize = wid * (ht+1) + 16;
    if ((pic->bitmap = (unsigned char*) malloc(bufsize)) == NULL) {
	    return 0;	/* couldn't alloc space for image */
    }

    buffer = pic->bitmap;

    switch (pcxhead.bppl) {
	case 1:   
	case 8:   
		pcx_decode(pcxfile, buffer, bufsize, pcxhead.bppl, &pcxhead, wid, ht);
		break;
	default:
		  fprintf(stderr,"Unsupported PCX format in %s",pic->file);
		  free(pic->bitmap);
		  return 0;
    }

    /* See if there is a VGA palette; read it into the pic->cmap */
    if (pcxhead.vers == 5) {
	fseek(pcxfile, -769L, SEEK_END);  /* backwards from end of file */

	if (getc(pcxfile) == 0x0C) {	/* VGA Palette ID value */ 
	    for (i = 0; i < 256; i++) {
		pic->cmap[0][i] = getc(pcxfile);
		pic->cmap[1][i] = getc(pcxfile);
		pic->cmap[2][i] = getc(pcxfile);
	    }
	    pic->numcols = 256;	/* for a VGA colormap */
	}
    }
    return 1;
}

unsigned short
getwrd(file)
FILE *file;
{
    unsigned char c1;
    c1 = getc(file);
    return (unsigned short) (c1 + (unsigned char) getc(file)*256);
}

void
readpcxhead(head, pcxfile)
pcxheadr	*head;
FILE		*pcxfile;
{
    register unsigned short i;

    head->id	= getc(pcxfile);
    head->vers	= getc(pcxfile);
    head->format = getc(pcxfile);
    head->bppl = getc(pcxfile);
    head->xmin	= getwrd(pcxfile);
    head->ymin	= getwrd(pcxfile);
    head->xmax	= getwrd(pcxfile);
    head->ymax	= getwrd(pcxfile);
    head->hdpi	= getwrd(pcxfile);
    head->vdpi	= getwrd(pcxfile);

    /* Read the EGA Palette */
    for (i = 0; i < sizeof(head->egapal); i++)
	head->egapal[i] = getc(pcxfile);

    head->reserv = getc(pcxfile);
    head->nplanes = getc(pcxfile);
    head->blp = getwrd(pcxfile); 
    head->palinfo = getwrd(pcxfile);  
    head->hscrnsiz = getwrd(pcxfile);  
    head->vscrnsiz = getwrd(pcxfile);

    /* Read the reserved area at the end of the header */
    for (i = 0; i < sizeof(head->fill); i++)
	head->fill[i] = getc(pcxfile);
}

void
pcx_decode(file, image, bufsize, planes, header, w, h)
    FILE     *file;
    unsigned  char *image;
    int       bufsize, planes;
    pcxheadr *header;
    int       w,h;
{
    int	      row, bcnt, bpl, pd;
    int       i, j, b, cnt;
    unsigned char mask, plane, pmsk;
    unsigned char *oimage;

    /* clear area first */
    bzero((char*)image,bufsize);
 
    bpl = header->blp;
    if (planes == 1)
	pd = (bpl * 8) - w;
    else
	pd = bpl - w;

    row = bcnt = 0;

    plane = 0;
    pmsk = 1;
    oimage = image;

    while ( (b=getc(file)) != EOF) {
	if ((b & 0xC0) == 0xC0) {   /* this is a repitition count */
	    cnt = b & 0x3F;
	    b = getc(file);
	    if (b == EOF) {
		getc(file);
		return; 
	    }
	} else
	    cnt = 1;
	
	for (i=0; i<cnt; i++) {
	    if (planes == 1) {
		for (j=0, mask=0x80; j<8; j++) {
		    *image++ |= (unsigned char) (((b & mask) ? pmsk : 0));
		    mask = mask >> 1;
		}
	    } else {
		*image++ = (unsigned char) b;
	    }
	
	    bcnt++;
	
	    if (bcnt == bpl) {     /* end of a scan line */
		bcnt = 0;
		plane++;  

		if (plane >= (int) header->nplanes) {   /* go to the next row */
		    plane = 0;
		    image -= pd;
		    oimage = image;
		    row++;
		    if (row >= h) {
			return;   /* done */
		    }
		} else {   /* new plane, same row */
			image = oimage;
		}	
	    pmsk = 1 << plane;
	    }
	}
    }
}
