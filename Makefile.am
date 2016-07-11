## Run automake on this file to produce Makefile.in
# This file is part of fig2dev - Translate Fig code to various devices.
#
# Copyright (c) 2016 Thomas Loimer <thomas.loimer@tuwien.ac.at>
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved. This file is offered as-is,
# without any warranty.

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = fig2dev man
if ENABLE_TRANSFIG
SUBDIRS += transfig
endif

EXTRA_DIST = CHANGES LATEX.AND.XFIG NOTES

MAINTAINERCLEANFILES = Makefile.in aclocal.m4 config.h.in configure

version.m4: configure.ac Makefile.am fig2dev/*.c fig2dev/*.h \
    fig2dev/dev/*.c fig2dev/dev/*.h
	if test `git describe` && test "`git describe --abbrev=1`" \
	    != "`sed -n '/FIG_V/ {s/.*\[\(.*\)\])/\1/;p}' $@`"; then \
	    sed -i "s/VERSION\], \[.*\]/VERSION], [`git describe --abbrev=1`]/" $@ \
	    && sed -i "s/DATE\], \[.*\]/DATE], [`LC_TIME=C date +'%b %Y' -r $@`]/" $@; \
	    fi
