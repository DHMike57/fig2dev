## Run automake on this file to produce Makefile.in

#
# Fig2dev: Translate fig code to various devices.
# Parts Copyright (c) 2016 by Thomas Loimer
#
# Any party obtaining a copy of these files is granted, free of charge, a
# full and unrestricted irrevocable, world-wide, paid up, royalty-free,
# nonexclusive right and license to deal in this software and
# documentation files (the "Software"), including without limitation the
# rights to use, copy, modify, merge, publish and/or distribute copies of
# the Software, and to permit persons who receive copies from any such
# party to do so, with the only requirement being that this copyright
# notice remain intact.
#

#
#	Makefile.am: Automake makefile
#	Author: Thomas Loimer <thomas.loimer@tuwien.ac.at>, 2016
#


ACLOCAL_AMFLAGS = -I m4

SUBDIRS = fig2dev man
if ENABLE_TRANSFIG
SUBDIRS += transfig
endif

EXTRA_DIST = CHANGES NOTES update-version_m4

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
