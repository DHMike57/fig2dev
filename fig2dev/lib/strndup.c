/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1991 by Micah Beck
 * Parts Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2002 by Brian V. Smith
 *
 * Any party obtaining a copy of these files is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and
 * documentation files (the "Software"), including without limitation the
 * rights to use, copy, modify, merge, publish and/or distribute copies of
 * the Software, and to permit persons who receive copies from any such
 * party to do so, with the only requirement being that this copyright
 * notice remain intact.
 *
 */

/*
 * strndup.c
 * Copied and modified from strdup.c, T. Loimer, 2015.
 * See strdup.c.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *
strndup(char const *str, size_t n)
{
	char	*p;

	if (!str)
		return (char *)NULL;

	p = (char *) malloc(n + 1);
	if (p != NULL) {
		strncpy(p, str, n);
		p[n] = '\0';
	}
	return p;
}
