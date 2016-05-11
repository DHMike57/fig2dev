## Run automake on this file to produce Makefile.in

MAINTAINERCLEANFILES = Makefile.in

## bitmapsdir = $(pkgdatadir)/bitmaps
## bitmapsdir is defined and set via AC_SUBST in configure.ac.

dist_bitmaps_DATA = bricks.bmp circles.bmp crosshatch30.bmp crosshatch45.bmp \
	crosshatch.bmp dash.bmp dashDot.bmp dashDotDot.bmp dashDotDotDot.bmp \
	dot.bmp fishscales.bmp hexagons.bmp horizontal.bmp horiz_saw.bmp \
	left30.bmp left45.bmp leftshingle.bmp octagons.bmp p13.bmp right30.bmp \
	right45.bmp rightshingle.bmp small_fishscales.bmp sp0.bmp sp10.bmp \
	sp11.bmp sp12.bmp sp13.bmp sp14.bmp sp15.bmp sp16.bmp sp17.bmp \
	sp18.bmp sp19.bmp sp1.bmp sp20.bmp sp2.bmp sp3.bmp sp4.bmp sp5.bmp \
	sp6.bmp sp7.bmp sp8.bmp sp9.bmp vert_bricks.bmp vertical.bmp \
	vert_leftshingle.bmp vert_rightshingle.bmp vert_saw.bmp

uninstall-hook:
	rmdir "$(DESTDIR)$(bitmapsdir)"
