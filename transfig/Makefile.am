## Run automake on this file to produce Makefile.in

MAINTAINERCLEANFILES = Makefile.in

# Additional libs are not needed for transfig (hopefully).
# Brute force, but otherwise -L/path/to/libs must also be provided.
LIBS =

# Silence some warnings
AM_CFLAGS = -Wno-parentheses

bin_PROGRAMS = transfig

transfig_SOURCES = transfig.c transfig.h txfile.c mkfile.c sys.c

transfig.$(OBJEXT): $(CONFIG_HEADER)

# transfig documentation is not installed, but only distributed
# with the source tarball
EXTRA_DIST = doc
