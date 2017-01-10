/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1991 by Micah Beck
 * Parts Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2016 by Brian V. Smith
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
 *	bool.h - created 2016-08-13 by Thomas Loimer
 *
 */

/* config.h must be read, before bool.h is included */

#ifndef BOOL_H
#define BOOL_H
/* proposed by info autoconf */
#ifdef HAVE_STDBOOL_H
#include <stdbool.h>
#else
#ifndef HAVE__BOOL
#ifdef __cplusplus
typedef bool	_Bool;
#else
#define	_Bool	signed char
#endif
#endif
#define	bool	_Bool
#define	false	0
#define	true	1
#define __bool_true_false_are_defined	1
#endif
#endif
