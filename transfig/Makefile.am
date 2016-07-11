## Run automake on this file to produce Makefile.in

MAINTAINERCLEANFILES = Makefile.in

# Additional libs are not needed for transfig (hopefully).
# Brute force, but otherwise -L/path/to/libs must also be provided.
LIBS =

bin_PROGRAMS = transfig

transfig_SOURCES = transfig.c transfig.h txfile.c mkfile.c sys.c

# transfig documentation not installed, but only distributed
# with the source tarball
EXTRA_DIST = doc
