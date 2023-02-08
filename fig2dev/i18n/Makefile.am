## Run automake on this file to produce Makefile.in

# Fig2dev: Translate Fig code to various Devices
# Copyright (c) 1991 by Micah Beck
# Parts Copyright (c) 1985-1988 by Supoj Sutanthavibul
# Parts Copyright (c) 1989-2015 by Brian V. Smith
# Parts Copyright (c) 2015-2023 by Thomas Loimer
#
# Any party obtaining a copy of these files is granted, free of charge, a
# full and unrestricted irrevocable, world-wide, paid up, royalty-free,
# nonexclusive right and license to deal in this software and documentation
# files (the "Software"), including without limitation the rights to use,
# copy, modify, merge, publish, distribute, sublicense and/or sell copies
# of the Software, and to permit persons who receive copies from any such
# party to do so, with the only requirement being that the above copyright
# and this permission notice remain intact.

# fig2dev/i18n/Makefile.am
# Author: Thomas Loimer


MAINTAINERCLEANFILES = Makefile.in

# With dist_, these files are installed into $(i18ndir).
dist_i18n_DATA = japanese.ps korean.ps

install-data-hook:
	cd "$(DESTDIR)$(i18ndir)" && { \
		for f in ja_JP.eucJP.ps ja_JP.EUC.ps ja_JP.ps ja_JP.ujis.ps ja.ps; do \
			if test -f $$f; then rm -f $$f; fi; \
			$(LN_S) japanese.ps $$f; done; \
		for f in ko_KR.eucKR.ps ko_KR.EUC.ps ko_KR.ps ko.ps; do \
			if test -f $$f; then rm -f $$f; fi; \
			$(LN_S) korean.ps $$f; done; }

## The dist_i18n_DATA files must be removed before the i18ndir can be removed.
## Therefore, use uninstall-hook, not uninstall-local (*-hook is guarantueed
## to follow on uninstall, uninstall-local might execute in between)
uninstall-hook:
	cd "$(DESTDIR)$(i18ndir)" && \
	rm -f ja_JP.eucJP.ps ja_JP.EUC.ps ja_JP.ps ja_JP.ujis.ps ja.ps \
	  ko_KR.eucKR.ps ko_KR.EUC.ps ko_KR.ps ko.ps && \
	cd ..
	rmdir "$(DESTDIR)$(i18ndir)"	# this dir should now be empty
