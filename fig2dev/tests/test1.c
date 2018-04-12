/*
 * Fig2dev: Translate Fig code to various Devices
 * Copyright (c) 1991 by Micah Beck
 * Parts Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2010 by Brian V. Smith
 * Parts Copyright (c) 2015-2017 by Thomas Loimer
 *
 * Any party obtaining a copy of these files is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and documentation
 * files (the "Software"), including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense and/or sell copies
 * of the Software, and to permit persons who receive copies from any such
 * party to do so, with the only requirement being that the above copyright
 * and this permission notice remain intact.
 *
 */

/*
 * test1.c: Try to read RGB_FILE and files in BITMAPDIR and I18N_DATADIR.
 * Author: Thomas Loimer, 2018-03-08
 *
 * Installation test.
 * Check, whether installation in strange paths, for instance
 * containing a backslash after configure --prefix='/tmp/strange\dir', works.
 * The STRINGIZE(S) is not used any longer, strange paths are not possible.
 * This is of more relevance for installation in, e.g.,
 * C:\Program Files (x86)\fig2dev\bitmaps.
 * The code below really mocks the code in genps.c, gentk.c, genptk.c and
 * colors.c and therefore must be kept in sync with the code there.
 * Integration tests would be to fragile: I18N_DATADIR depends on the locale
 * to be present, gentk.c and genptk.c simply write the file paths into the
 * output file. RGB_FILE is checked in an integration test.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define STRINGIZE(S) _STRINGIZE(S)
#define _STRINGIZE(S) #S

int
main(void)
{
	const char mode[] = "rb";
	int err = 0;
	char buf[BUFSIZ];
	char *filepath = buf;
//	char rgbpath[] = STRINGIZE(RGB_FILE);
//	char bitmapdir[] = STRINGIZE(BITMAPDIR);
//	char i18ndir[] = STRINGIZE(I18N_DATADIR);
	char rgbpath[] = RGB_FILE;
	char bitmapdir[] = BITMAPDIR;
	char i18ndir[] = I18N_DATADIR;
	size_t	n;

	n = strlen(rgbpath);
//	rgbpath[n-1] = '\0';
	if (fopen(rgbpath, mode) == NULL) {
		printf("Cannot open file: %s\n", rgbpath);
		err += 1;
	}

	n = strlen(bitmapdir);
	if (n + 9 > BUFSIZ)
		filepath = malloc(n + 9);
//	memcpy(filepath, bitmapdir + 1, n - 2);
//	memcpy(filepath + n - 2, "/sp0.bmp", (size_t)9);
	memcpy(filepath, bitmapdir, n);
	memcpy(filepath + n, "/sp0.bmp", (size_t)9);
	if (fopen(filepath, mode) == NULL) {
		printf("Cannot open file: %s\n", filepath);
		err += 2;
	}

#ifdef I18N_DATADIR
	n = strlen(i18ndir);
	if (n + 10 > BUFSIZ)
		filepath = malloc(n + 10);
//	memcpy(filepath, i18ndir + 1, n - 2);
//	memcpy(filepath + n - 2, "/cs_CZ.ps", (size_t)10);
	memcpy(filepath, i18ndir, n);
	memcpy(filepath + n, "/cs_CZ.ps", (size_t)10);
	if (fopen(filepath, mode) == NULL) {
		printf("Cannot open file: %s\n", filepath);
		err += 4;
	}
#endif

	if (filepath != buf)
		free(filepath);
	return err;
}
