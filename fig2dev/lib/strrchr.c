/*
 * 2015-03-05:	Created, adapted from strchr.c.
 *
 */

char *
strrchr(register char *s, register int c)
{
	char	*val = NULL;
	while (*s++)
		if (c == *s) val = s;
	return (val);
}
