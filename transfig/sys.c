/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1985 Supoj Sutantavibul
 * Copyright (c) 1991 Micah Beck
 *
 * THE AUTHORS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * The X Consortium, and any party obtaining a copy of these files from
 * the X Consortium, directly or indirectly, is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and
 * documentation files (the "Software"), including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons who receive
 * copies from any such party to do so, with the only requirement being
 * that this copyright notice remain intact.  This license includes without
 * limitation a license to do the foregoing actions under any patents of
 * the party supplying this software to the X Consortium.
 */

#include <stdio.h>
#include "transfig.h"

#define MAXSYS 10000
static char sysbuf[MAXSYS];

char *sysls()
{
  FILE *ls;
  int i;
  char c;

  ls = popen("/bin/ls *.fig", "r");
  i = 0;
  c = fgetc(ls);
  while (c != EOF & i < MAXSYS-1)
  {
	sysbuf[i] = c;
	i += 1;
	c = fgetc(ls);
  }
  sysbuf[i] = '\0';
  return sysbuf;
}

sysmv(f)
char *f;
{
  sprintf(sysbuf, "%s~", f);
  unlink(sysbuf);
  if (!link(f, sysbuf)) unlink(f);
}

char *strip(str, suf)
char *str, *suf;
{
  char *p1, *p2;

  for (p1 = &str[strlen(str)-1], p2 = &suf[strlen(suf)-1];
	(p1 >= str && p2 >= suf) && (*p1 == *p2);
	--p1, --p2);

  if (p2 < suf)
  {
	*(p1+1) = '\0';
	return str;
  } else
	return NULL;
}

char *mksuff(name, suff)
char *name, *suff;
{
  char *temp;

  temp = (char *)malloc(strlen(name)+strlen(suff)+1);
  strcpy(temp, name);
  strcat(temp, suff);
  return temp;
}
