/*
 * Fig2dev: Translate Fig code to various Devices
 * Copyright (c) 1991 by Micah Beck
 * Parts Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2015 by Brian V. Smith
 * Parts Copyright (c) 2015-2023 by Thomas Loimer
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

#ifndef TEXTCONVERT_H
#define TEXTCONVERT_H

#if defined HAVE_CONFIG_H && !defined VERSION
#include "config.h"
#endif
#include <stddef.h>		/* size_t */

extern char	*conv_non_ascii(const char *restrict str);
extern char	*conv_textisutf8(const char *restrict str);
extern int	conv_textstring(char **restrict out, char *restrict in,
								size_t inlen);
extern int	conv_filename(char **restrict out, char *restrict in,
								size_t inlen);
extern int	convertutf8tolatin1(char *restrict str);
#endif
