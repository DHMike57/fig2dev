## Run automake on this file to produce Makefile.in

MAINTAINERCLEANFILES = Makefile.in

# Install the given files in section 1 of the man-pages.
# man_MANS would deduce the correct section from the file suffix, at a
# slight speed penalty.
# Without the dist_ prefix these files are not copied into the tarball
# created by "make dist", because automake thinks man-pages are generated
# from other sources.
dist_man1_MANS = fig2ps2tex.1 pic2tpic.1
# fig2dev.1 _is_ created from fig2dev.1.in, i18n.man and nfss.man.
# fig2dev.1.in is mentioned in configure.ac, so no need
# to mention it here in EXTRA_DIST
# But mention transfig.1, otherwise it is not distributed, if configure
# runs without --enable-transfig.
man1_MANS = fig2dev.1
EXTRA_DIST = i18n.man nfss.man transfig.1

if ENABLE_TRANSFIG
man1_MANS += transfig.1
endif

uninstall-hook:
	-rmdir "$(DESTDIR)$(man1dir)"
