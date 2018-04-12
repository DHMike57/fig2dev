## Run automake on this file to produce Makefile.in

# Fig2dev: Translate Fig code to various Devices
# Parts Copyright (c) 2016-2018 by Thomas Loimer
#
# Any party obtaining a copy of these files is granted, free of charge, a
# full and unrestricted irrevocable, world-wide, paid up, royalty-free,
# nonexclusive right and license to deal in this software and documentation
# files (the "Software"), including without limitation the rights to use,
# copy, modify, merge, publish, distribute, sublicense and/or sell copies
# of the Software, and to permit persons who receive copies from any such
# party to do so, with the only requirement being that the above copyright
# and this permission notice remain intact.

# fig2dev/tests/Makefile.am
# Author: Thomas Loimer, 2016-2018.


# testsuite and package.m4 must be distributed, hence they are
# written into the source tree
TESTSUITE = $(srcdir)/testsuite
EXTRA_DIST = testsuite package.m4 $(TESTSUITE_AT) atlocal.in data
TESTSUITE_AT = testsuite.at bitmaps.at input.at output.at read.at

# ??
DISTCLEANFILES = atconfig
MAINTAINERCLEANFILES = Makefile.in $(TESTSUITE) $(srcdir)/package.m4

$(srcdir)/package.m4: $(top_srcdir)/configure.ac
	:;{ \
	  echo 'm4_define([AT_PACKAGE_NAME], [@PACKAGE_NAME@])'; \
	  echo 'm4_define([AT_PACKAGE_TARNAME], [@PACKAGE_TARNAME@])'; \
	  echo 'm4_define([AT_PACKAGE_VERSION], [@PACKAGE_VERSION@])'; \
	  echo 'm4_define([AT_PACKAGE_STRING], [@PACKAGE_STRING@])'; \
	  echo 'm4_define([AT_PACKAGE_BUGREPORT], [@PACKAGE_BUGREPORT@])'; \
	  echo 'm4_define([AT_PACKAGE_URL], [@PACKAGE_URL@])'; \
	} >'$(srcdir)/package.m4'

check_PROGRAMS = test1

# keep the definition below in sync with that in ../Makefile.am
AM_CPPFLAGS = -DRGB_FILE="\"$(pkgdatadir)/rgb.txt\""
# keep the definitions below in sync with those in ../dev/Makefile.am
AM_CPPFLAGS += -DBITMAPDIR="\"$(bitmapsdir)\""
if ENABLE_I18N
AM_CPPFLAGS += -DI18N_DATADIR="\"$(i18ndir)\""
endif

check-local: atconfig $(TESTSUITE) atlocal
	$(SHELL) '$(TESTSUITE)' INSTALLCHECK=no $(TESTSUITEFLAGS)

installcheck-local: atconfig $(TESTSUITE) atlocal
	$(SHELL) '$(TESTSUITE)' AUTOTEST_PATH='$(bindir)' $(TESTSUITEFLAGS)

clean-local:
	test ! -f '$(TESTSUITE)' || \
		$(SHELL) '$(TESTSUITE)' --clean

AUTOTEST = $(AUTOM4TE) --language=autotest

$(TESTSUITE): $(srcdir)/package.m4 $(TESTSUITE_AT)
	$(AUTOTEST) -I '$(srcdir)' -o $@.tmp $@.at
	mv $@.tmp $@
