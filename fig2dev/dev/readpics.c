/*
 * TransFig: Facility for Translating Fig code
 *
 * Various copyrights in this file follow
 * Parts Copyright (c) 1994 Brian V. Smith
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
#include <sys/types.h>
#include <sys/stat.h>
#include "fig2dev.h"

/* define PATH_MAX if not already defined */
/* taken from the X11R5 server/os/osfonts.c file */
#ifndef X_NOT_POSIX
#ifdef _POSIX_SOURCE
#include <limits.h>
#else
#if !defined(sun) || defined(sparc)
#define _POSIX_SOURCE
#include <limits.h>
#undef _POSIX_SOURCE
#endif /* !defined(sun) || defined(sparc) */
#endif /* _POSIX_SOURCE */
#endif /* X_NOT_POSIX */

#ifndef PATH_MAX
#include <sys/param.h>
#ifdef MAXPATHLEN
#define PATH_MAX MAXPATHLEN
#else
#define PATH_MAX 1024
#endif /* MAXPATHLEN */
#endif /* PATH_MAX */


FILE *
open_picfile(name, type)
    char	*name;
    int		*type;
{
    char	unc[PATH_MAX+20];	/* temp buffer for uncompress/gunzip command */
    char	*compname;
    FILE	*fstream;		/* handle on file  */
    struct stat	status;

    *type = 0;
    compname = NULL;
    /* see if the filename ends with .Z */
    /* if so, generate uncompress command and use pipe (filetype = 1) */
    if (strlen(name) > (size_t)2 && !strcmp(".Z", name + (strlen(name)-2))) {
	sprintf(unc,"uncompress -c %s",name);
	*type = 1;
    /* or with .z or .gz */
    } else if ((strlen(name) > (size_t)3 && !strcmp(".gz", name + (strlen(name)-3))) ||
	      ((strlen(name) > (size_t)2 && !strcmp(".z", name + (strlen(name)-2))))) {
	sprintf(unc,"gunzip -qc %s",name);
	*type = 1;
    /* none of the above, see if the file with .Z or .gz or .z appended exists */
    } else {
	compname = (char*) malloc(strlen(name)+4);
	strcpy(compname, name);
	strcat(compname, ".Z");
	if (!stat(compname, &status)) {
	    sprintf(unc, "uncompress -c %s",compname);
	    *type = 1;
	    name = compname;
	} else {
	    strcpy(compname, name);
	    strcat(compname, ".z");
	    if (!stat(compname, &status)) {
		sprintf(unc, "gunzip -c %s",compname);
		*type = 1;
		name = compname;
	    } else {
		strcpy(compname, name);
		strcat(compname, ".gz");
		if (!stat(compname, &status)) {
		    sprintf(unc, "gunzip -c %s",compname);
		    *type = 1;
		    name = compname;
		}
	    }
	}
    }
    /* no appendages, just see if it exists */
    if (stat(name, &status) != 0) {
	fstream = NULL;
    } else {
	switch (*type) {
	  case 0:
	    fstream = fopen(name, "r");
	    break;
	  case 1:
	    fstream = popen(unc,"r");
	    break;
	}
    }
    if (compname)
	free(compname);
    return fstream;
}

void
close_picfile(file,type)
    FILE	*file;
    int		type;
{
    if (type == 0)
	fclose(file);
    else
	pclose(file);
}
