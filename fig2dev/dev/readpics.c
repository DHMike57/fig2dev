/*
 * TransFig: Facility for Translating Fig code
 * Various copyrights in this file follow
 * Parts Copyright (c) 1994-1999 by Brian V. Smith
 *
 * Any party obtaining a copy of these files is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and
 * documentation files (the "Software"), including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons who receive
 * copies from any such party to do so, with the only requirement being
 * that this copyright notice remain intact.
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include "fig2dev.h"

/* 
   Open the file 'name' and return its type (pipe or real file) in 'type'.
   Return the full name in 'retname'.  This will have a .gz or .Z if the file is
   zipped/compressed.
   The return value is the FILE stream.
*/

FILE *
open_picfile(name, type, pipeok, retname)
    char	*name;
    int		*type;
    Boolean	 pipeok;
    char	*retname;
{
    char	 unc[PATH_MAX+20];	/* temp buffer for uncompress/gunzip command */
    FILE	*fstream;		/* handle on file  */
    struct stat	 status;
    char	*gzoption;

    *type = 0;
    *retname = '\0';
    if (pipeok)
	gzoption = "-c";		/* tell gunzip to output to stdout */
    else
	gzoption = "";

    /* see if the filename ends with .Z */
    /* if so, generate uncompress command and use pipe (filetype = 1) */
    if (strlen(name) > 2 && !strcmp(".Z", name + (strlen(name)-2))) {
	sprintf(unc,"uncompress %s %s", gzoption, name);
	*type = 1;
    /* or with .z or .gz */
    } else if ((strlen(name) > 3 && !strcmp(".gz", name + (strlen(name)-3))) ||
	      ((strlen(name) > 2 && !strcmp(".z", name + (strlen(name)-2))))) {
	sprintf(unc,"gunzip -q %s %s", gzoption, name);
	*type = 1;
    /* none of the above, see if the file with .Z or .gz or .z appended exists */
    } else {
	strcpy(retname, name);
	strcat(retname, ".Z");
	if (!stat(retname, &status)) {
	    sprintf(unc, "uncompress %s %s", gzoption, retname);
	    *type = 1;
	    name = retname;
	} else {
	    strcpy(retname, name);
	    strcat(retname, ".z");
	    if (!stat(retname, &status)) {
		sprintf(unc, "gunzip %s %s", gzoption, retname);
		*type = 1;
		name = retname;
	    } else {
		strcpy(retname, name);
		strcat(retname, ".gz");
		if (!stat(retname, &status)) {
		    sprintf(unc, "gunzip %s %s", gzoption, retname);
		    *type = 1;
		    name = retname;
		}
	    }
	}
    }
    /* if a pipe, but the caller needs a file, uncompress the file now */
    if (*type == 1 && !pipeok) {
	char *p;
	system(unc);
	if (p=strrchr(name,'.')) {
	    *p = '\0';		/* terminate name before last .gz, .z or .Z */
	}
	strcpy(retname, name);
	/* force to plain file now */
	*type = 0;
    }

    /* no appendages, just see if it exists */
    /* and restore the original name */
    strcpy(retname, name);
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
