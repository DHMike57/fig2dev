## Run automake on this file to produce Makefile.in

MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = dev bitmaps
if ENABLE_I18N
SUBDIRS += i18n
endif
SUBDIRS += tests

# Copy the rgb.txt file into, usually, ../share/fig2dev/rgb.txt
## The pkgdata has its argument be put into pkgdatadir
dist_pkgdata_DATA = rgb.txt

## Need to use \"$(DESTDIR)$(pkgdatadir)? ... NO!
AM_CPPFLAGS = -DRGB_FILE=\"$(pkgdatadir)/rgb.txt\"
AM_CFLAGS = -Wno-parentheses

bin_PROGRAMS = fig2dev

fig2dev_SOURCES = alloc.h arrow.c bound.h bound.c colors.c drivers.h fig2dev.h \
    fig2dev.c free.h free.c iso2tex.c localmath.h localmath.c object.h \
    psfonts.c read1_3.c read.h read.c trans_spline.h trans_spline.c \
    pi.h getopt.c

## LIBOBJS may contain, e.g., strstr.o, since configure.ac contains
## AC_REPLACE_FUNCS(strstr) -- but usually LIBOBJS will be empty
fig2dev_LDADD = $(LIBOBJS) dev/libdrivers.a

dist_bin_SCRIPTS = fig2ps2tex pic2tpic

uninstall-hook:
	rmdir "$(DESTDIR)$(pkgdatadir)"
