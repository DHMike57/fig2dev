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

/* The following code is from the example.c file in the JPEG distribution */

#include "fig2dev.h"
#include "object.h"
#include "setjmp.h"
#include <sys/types.h>
#include "jpeglib.h"

static	Boolean	read_JPEG_file();

static	F_pic	   *pict;
static	unsigned char *bitmapptr;

/* return codes:  1 : success
		  0 : failure
*/

int
read_jpg(file, filetype, pic, llx, lly)
    FILE	   *file;
    int		    filetype;
    F_pic	   *pic;
    int		   *llx, *lly;
{

	*llx = *lly = 0;
	pict = pic;			/* for global access to the object */
	if (!read_JPEG_file(file))
	    return 0;

	/* output PostScript comment */
	fprintf(tfp, "%% Begin Imported JPEG File: %s\n\n", pic->file);

	/* number of colors, size and bitmap is put in by read_JPEG_file() */
	pic->subtype = P_JPEG;
	return 1;
}

/* These static variables are needed by the error routines. */

static	jmp_buf setjmp_buffer;		/* for return to caller */
static	void	error_exit();

struct error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct error_mgr * error_ptr;

/*
 * OK, here is the main function that actually causes everything to happen.
 * We assume here that the JPEG file is already open and that all
 * decompression parameters can be default values.
 * The routine returns TRUE if successful, FALSE if not.
 */

static Boolean
read_JPEG_file (file)
   FILE  *file;
{
	int i;

	/* This struct contains the JPEG decompression parameters and pointers to
	 * working space (which is allocated as needed by the JPEG library).
	 */
	struct jpeg_decompress_struct cinfo;

	JSAMPARRAY buffer;	/* Output row buffer */
	int row_stride;		/* physical row width in output buffer */
	struct error_mgr jerr;	/* error handler */

	/* Step 1: allocate and initialize JPEG decompression object */

	/* We set up the normal JPEG error routines, then override error_exit. */
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = error_exit;

	/* Establish the setjmp return context for error_exit to use. */
	if (setjmp(jerr.setjmp_buffer)) {
	  /* If we get here, the JPEG code has signaled an error.
	   * We need to clean up the JPEG object and return.
	   */
	  jpeg_destroy_decompress(&cinfo);
	  return FALSE;
	}
	/* Now we can initialize the JPEG decompression object. */
	jpeg_create_decompress(&cinfo);

	/* Step 2: specify data source (i.e. our file) */

	jpeg_stdio_src(&cinfo, file);

	/* Step 3: read file parameters with jpeg_read_header() */

	(void) jpeg_read_header(&cinfo, TRUE);
	/* We can ignore the return value from jpeg_read_header since
	 *   (a) suspension is not possible with the stdio data source, and
	 *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
	 * See libjpeg.doc for more info.
	 */

	/* We want a colormapped color space */
	/* Let the jpeg library do a two-pass over the image to make nice colors */
	cinfo.quantize_colors = TRUE;

	/* Now fill in the pict parameters */

	pict->bit_size.x = cinfo.image_width;
	pict->bit_size.y = cinfo.image_height;
	if ((pict->bitmap = (unsigned char *) 
	     malloc(cinfo.image_width * cinfo.image_height)) == NULL)
		error_exit("Can't alloc memory for JPEG image");
	bitmapptr = pict->bitmap;

	/* Step 4: set parameters for decompression */

	/* In this example, we don't need to change any of the defaults set by
	 * jpeg_read_header(), so we do nothing here.
	 */

	/* Step 5: Start decompressor */

	jpeg_start_decompress(&cinfo);

	/* We may need to do some setup of our own at this point before reading
	 * the data.  After jpeg_start_decompress() we have the correct scaled
	 * output image dimensions available, as well as the output colormap
	 * if we asked for color quantization.
	 * In this example, we need to make an output work buffer of the right size.
	 */ 
	/* JSAMPLEs per row in output buffer */
	row_stride = cinfo.output_width * cinfo.output_components;
	/* Make a one-row-high sample array that will go away when done with image */
	buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

	/* Step 6: while (scan lines remain to be read) */
	/*           jpeg_read_scanlines(...); */

	/* Here we use the library's state variable cinfo.output_scanline as the
	 * loop counter, so that we don't have to keep track ourselves.
	 */
	while (cinfo.output_scanline < cinfo.output_height) {
	  (void) jpeg_read_scanlines(&cinfo, buffer, 1);
	  for (i = 0; i < row_stride; i++)
		*bitmapptr++ = (unsigned char) buffer[0][i];
	}

	/* Step 7: fill up the colortable in the pict object */
	/* (Must do this before jpeg_finish_decompress or jpeg_destroy_decompress) */

	pict->numcols = cinfo.actual_number_of_colors;
	for (i = 0; i < pict->numcols; i++) {
	    pict->cmap[0][i] = cinfo.colormap[0][i];
	    /* set other colors to first if grayscale */
	    if (cinfo.jpeg_color_space == JCS_GRAYSCALE) {
		pict->cmap[1][i] = pict->cmap[2][i] = pict->cmap[0][i];
	    } else {
		pict->cmap[1][i] = cinfo.colormap[1][i];
		pict->cmap[2][i] = cinfo.colormap[2][i];
	    }
	}

	/* Step 8: Finish decompression */

	(void) jpeg_finish_decompress(&cinfo);
	/* We can ignore the return value since suspension is not possible
	 * with the stdio data source.
	 */

	/* Step 9: Release JPEG decompression object */

	/* This is an important step since it will release a good deal of memory. */
	jpeg_destroy_decompress(&cinfo);

	/* At this point you may want to check to see whether any corrupt-data
	 * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
	 */

	/* And we're done! */
	return TRUE;
}

/*
 * Here's the routine that will replace the standard error_exit method:
 */

static void
error_exit ( cinfo )
j_common_ptr cinfo;
{
    char buffer[JMSG_LENGTH_MAX];

    /* cinfo->err really points to a error_mgr struct, so coerce pointer */
    error_ptr err = (error_ptr) cinfo->err;

    /* Display the message if it is NOT "Not a JPEG file" */
    /* However, since the error number is not public we have to parse the string */

    /* Format the message */
    (*cinfo->err->format_message) (cinfo, buffer);

    if (strncmp(buffer,"Not a JPEG file",15)!=0)
	put_msg("%s", buffer);

    /* Return control to the setjmp point */
    longjmp(err->setjmp_buffer, 1);
}
