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

EXTRA_DIST = CHANGES NOTES

MAINTAINERCLEANFILES = Makefile.in aclocal.m4 config.h.in configure

if ENABLE_VERSIONING
# The generated Makefile lets configure and config.h.in depend on configure.ac,
# and on all .m4 files. If configure.ac is changed, configure is re-created,
# then config.h, probably only after version.m4 is updated, and configure
# is again recreated and run. Only a nuisance, though.
version.m4: configure.ac Makefile.am \
	fig2dev/*.c fig2dev/*.h fig2dev/dev/*.c fig2dev/dev/*.h
	. ./update-version_m4
endif
