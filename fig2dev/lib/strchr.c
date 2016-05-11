/* this is included because index is not on some UNIX systems */
/*
 * 2015-03-05:	Created, cut from getopt.c.
 *
 */

char *
strchr(register char *s, register int c)
{
	while (*s)
		if (c == *s) return (s);
		else ++s;
	return (NULL);
}
