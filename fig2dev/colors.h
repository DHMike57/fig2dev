/*
 * Fig2dev: Translate Fig code to various Devices
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
 * colors.h
 * Written by Thomas Loimer, 2016-07-07
 *
 */

extern int	lookup_X_color(char *name, RGB *rgb);
extern float	rgb2luminance (float r, float g, float b);
