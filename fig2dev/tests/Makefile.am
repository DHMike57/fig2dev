# testsuite and package.m4 must be distributed, hence they are
# written into the source tree
TESTSUITE = $(srcdir)/testsuite
EXTRA_DIST = testsuite package.m4 $(TESTSUITE_AT) atlocal.in data
TESTSUITE_AT = testsuite.at arrows_eps.at lookup_X_color.at read_sanitize.at \
	pict2e.at tikz.at svg.at creationdate.at bitmaps.at
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

#  echo 'm4_define([RGB_FILE], [$(pkgdatadir)/rgb.txt])';

# was:	$(SHELL) '$(TESTSUITE)' RGB_FILE='$(pkgdatadir)/rbg.txt' ...
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
