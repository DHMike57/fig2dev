## Run automake on this file to produce Makefile.in

# Fig2dev: Translate Fig code to various Devices
# Parts Copyright (c) 2015-2017 by Thomas Loimer
#
# Any party obtaining a copy of these files is granted, free of charge, a
# full and unrestricted irrevocable, world-wide, paid up, royalty-free,
# nonexclusive right and license to deal in this software and documentation
# files (the "Software"), including without limitation the rights to use,
# copy, modify, merge, publish, distribute, sublicense and/or sell copies
# of the Software, and to permit persons who receive copies from any such
# party to do so, with the only requirement being that the above copyright
# and this permission notice remain intact.

# Makefile.am
# Author: Thomas Loimer, 2015-2017.


ACLOCAL_AMFLAGS = -I m4

SUBDIRS = fig2dev man
if ENABLE_TRANSFIG
SUBDIRS += transfig
endif

EXTRA_DIST = CHANGES NOTES update-version_m4

MAINTAINERCLEANFILES = Makefile.in aclocal.m4 config.h.in configure
