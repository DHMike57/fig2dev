/*
 * Fig2dev: Translate Fig code to various Devices
 * Copyright (c) 1991 by Micah Beck
 * Parts Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2015 by Brian V. Smith
 * Parts Copyright (c) 2015-2019 by Thomas Loimer
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
 * readpics.c: read image files
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "readpics.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "fig2dev.h"	/* includes "bool.h" */
#include "messages.h"
#include "xtmpfile.h"

/*
 * Open the file 'name' and return its type (real file=0, pipe=1) in 'type'.
 * Return the full name in the buffer pointed to by 'retname'.
 * The caller must free(*retname) after calling open_picfile()!
 * 'retname' will have a .gz, .z or .Z if the file is zipped/compressed. If the
 * caller cannot take a pipe (pipeok=false), and the file is compressed,
 * uncompress and return name with .z, .Z or .gz stripped.
 * The return value is the FILE stream.
 */

FILE *
open_picfile(char *name, int *type, bool pipeok, char **retname)
{
    char	*unc;			/* temp buffer for gunzip command */
    FILE	*fstream;		/* handle on file  */
    struct stat	 status;
    size_t	len;
    size_t	pos;

    *type = 0;
    len = strlen(name);
    *retname = malloc(len + 4);
//  *retname[0] = '\0';
    if (pipeok) {
	unc = malloc(len + 17);		/* "gunzip -q -c " name ".gz" */
	strcpy(unc, "gunzip -q -c ");	/* tell gunzip to output to stdout */
    } else {
	unc = malloc(len + 14);		/* "gunzip -q " name ".gz" */
	strcpy(unc, "gunzip -q ");
    }
    pos = strlen(unc);

    if (!stat(name, &status)) {
	/* see if the filename ends with .Z or .z or .gz */
	if ((len > 3 && !strcmp(".gz", name + (len-3))) ||
		    (len > 2 && (!strcmp(".z", name + (len-2)) ||
				 !strcmp(".Z", name + (len-2))))) {
	/* yes, make command to uncompress it */
	memcpy(unc + pos, name, len + 1);	/* strcat(unc, name) */
	*type = 1;
	} else {
	    /* use straight name */
	    *type = 0;
	}
	memcpy(*retname, name, len + 1);    /* strcpy(*retname, name) */
    } else {
	/* no, see if the file with .gz, .z or .Z appended exists */
	/* failing that, if there is an absolute path, strip it and
	   look in current directory */
	/* check for .gz */
	memcpy(*retname, name, len);
	memcpy(*retname + len, ".gz", (size_t) 4); /* retname = name".gz" */
	if (!stat(*retname, &status)) {
	    /* yes, found with .gz */
	    memcpy(unc + pos, *retname, len + 4);
	    *type = 1;
	} else {
	    /* no, check for .z */
	    memcpy(*retname + len, ".z", (size_t) 3);
	    if (!stat(*retname, &status)) {
		/* yes, found with .z */
		memcpy(unc + pos, *retname, len + 3);
		*type = 1;
	    } else {
		/* no, check for .Z */
		memcpy(*retname + len, ".Z", (size_t) 3);
		if (!stat(*retname, &status)) {
		    /* yes, found with .Z */
		    memcpy(unc + pos, *retname, len + 3);
		    *type = 1;
		} else {
		    char *p;
		    /* can't find it, if there is a path,
		       strip it and look in current directory */
		    if ((p = strrchr(name, '/'))) {
			/* yes, strip it off */
			/* strcpy(*retname, p + 1); */
			memcpy(*retname, p + 1, len -= (p - name));
			if (!stat(*retname, &status)) {
			    *type = 0;
			    memcpy(name, *retname, len);
			} else {
			    /* All is lost */
			    free(unc);
			    return NULL;
			}
		    }
		}
	    }
	}
    }

    /* if a pipe, but the caller needs a file, uncompress the file now */
    if (*type == 1 && !pipeok) {
	char *p;
	system(unc);
	if ((p = strrchr(*retname,'.'))) {
	    *p = '\0';		/* terminate name before last .gz, .z or .Z */
	}
	strcpy(name, *retname);
	/* force to plain file now */
	*type = 0;
    }

    if (*type == 0)
	fstream = fopen(name, "rb");
    else  /* *type == 1 */
	fstream = popen(unc, "r");

    free(unc);
    return fstream;
}

void
close_picfile(FILE *file, int type)
{
    if (type == 0)
	fclose(file);
    else
	pclose(file);
}

void
init_stream(struct xfig_stream *restrict xf_stream)
{
	xf_stream->fp = NULL;
	xf_stream->name = xf_stream->name_buf;
	xf_stream->name_on_disk = xf_stream->name_on_disk_buf;
	xf_stream->uncompress = NULL;
	xf_stream->content = xf_stream->content_buf;
	*xf_stream->content = '\0';
}

void
free_stream(struct xfig_stream *restrict xf_stream)
{
	if (xf_stream->content != xf_stream->name_on_disk) {
		if (*xf_stream->content && unlink(xf_stream->content)) {
			err_msg("Cannot remove temporary file %s",
					xf_stream->content);
		}
		if (xf_stream->content != xf_stream->content_buf)
			free(xf_stream->content);
	}
	if (xf_stream->name != xf_stream->name_buf)
		free(xf_stream->name);
	if (xf_stream->name_on_disk != xf_stream->name_on_disk_buf)
		free(xf_stream->name_on_disk);
}

/*
 * Given name, search and return the name of the corresponding file on disk in
 * found. This may be "name", or "name" with a compression suffix appended,
 * e.g., "name.gz". If the file must be uncompressed, return the compression
 * command in a static string pointed to by "uncompress", otherwise let
 * "uncompress" point to the empty string. If the size of the character buffer
 * provided in found is too small to accomodate the string, return a pointer to
 * a malloc()'ed string.
 * Return 0 on success, -1 on failure.
 * Usage:
 *	char	found_buf[len];
 *	char	*found = found_buf;
 *	file_on_disk(name, &found, sizeof found_buf, &uncompress);
 *	...
 *	if (found != found_buf)
 *		free(found);
 */
static int
file_on_disk(char *restrict name, char *restrict *found, size_t len,
		const char *restrict *uncompress)
{
	int		i;
	size_t		name_len;
	char		*suffix;
	struct stat	status;
	static const char empty[] = "";
	static const char *filetypes[][2] = {
		/* sorted by popularity? */
#define FILEONDISK_ADD	5	/* must be max(strlen(filetypes[0][])) + 1 */
		{ ".gz",	"gunzip -c" },
		{ ".Z",		"gunzip -c" },
		{ ".z",		"gunzip -c" },
		{ ".zip",	"unzip -p"  },
		{ ".bz2",	"bunzip2 -c" },
		{ ".bz",	"bunzip2 -c" },
		{ ".xz",	"unxz -c" }
	};
	const int	filetypes_len =
				(int)(sizeof filetypes / sizeof(filetypes[1]));

	name_len = strlen(name);
	if (name_len >= len &&
			(*found = malloc(name_len + FILEONDISK_ADD)) == NULL) {
		put_msg(Err_mem);
		return -1;
	}

	strcpy(*found, name);

	/*
	 * Possibilities, e.g.,
	 *	name		name on disk		uncompress
	 *	img.ppm		img.ppm			""
	 *	img.ppm		img.ppm.gz		gunzip -c
	 *	img.ppm.gz	img.ppm.gz		gunzip -c
	 *	img.ppm.gz	img.ppm			""
	 */
	if (stat(name, &status)) {
		/* File not found. Now try, whether a file with one of
		   the known suffices appended exists. */
		if (len < name_len + FILEONDISK_ADD && (*found =
				malloc(name_len + FILEONDISK_ADD)) == NULL) {
			put_msg(Err_mem);
			return -1;
		}
		suffix = *found + name_len;
		for (i = 0; i < filetypes_len; ++i) {
			strcpy(suffix, filetypes[i][0]);
			if (!stat(*found, &status)) {
				*uncompress = filetypes[i][1];
				break;
			}
		}

		/* Not found. Check, whether the file has one of the known
		   compression suffices, but the uncompressed file
		   exists on disk. */
		*suffix = '\0';
		if (i == filetypes_len && (suffix = strrchr(name, '.'))) {
			for (i = 0; i < filetypes_len; ++i) {
				if (!strcmp(suffix, filetypes[i][0])) {
					*(*found + (suffix - name)) = '\0';
					if (!stat(*found, &status)) {
						*uncompress = empty;
						break;
					} else {
						*found[0] = '\0';
						i = filetypes_len;
					}
				}
			}
		}

		if (i == filetypes_len) {
			/* not found */
			*found[0] = '\0';
			return -1;
		}
	} else {
		/* File exists. Check, whether the name has one of the known
		   compression suffices. */
		if ((suffix = strrchr(name, '.'))) {
			for (i = 0; i < filetypes_len; ++i) {
				if (!strcmp(suffix, filetypes[i][0])) {
					*uncompress = filetypes[i][1];
					break;
				}
			}
		} else {
			i = filetypes_len;
		}

		if (i == filetypes_len)
			*uncompress = empty;
	}

	return 0;
}

/*
 * Return a file stream, either to a pipe or to a regular file.
 * If xf_stream->uncompress[0] == '\0', it is a regular file, otherwise a pipe.
 */
FILE *
open_stream(char *restrict name, struct xfig_stream *restrict xf_stream)
{
	size_t	len;

	if (xf_stream->name != name) {
		/* strcpy (xf_stream->name, name) */
		len = strlen(name);
		if (len >= sizeof xf_stream->name_buf) {
			if ((xf_stream->name = malloc(len + 1)) == NULL) {
				put_msg(Err_mem);
				return NULL;
			}
		}
		memcpy(xf_stream->name, name, len + 1);
	}

	if (file_on_disk(name, &xf_stream->name_on_disk,
				sizeof xf_stream->name_on_disk_buf,
				&xf_stream->uncompress)) {

		free_stream(xf_stream);
		return NULL;
	}

	if (*xf_stream->uncompress) {
		/* a compressed file */
		char	command_buf[256];
		char	*command = command_buf;

		len = strlen(xf_stream->name_on_disk) +
					strlen(xf_stream->uncompress) + 2;
		if (len > sizeof command_buf) {
			if ((command = malloc(len)) == NULL) {
				put_msg(Err_mem);
				return NULL;
			}
		}
		sprintf(command, "%s %s",
				xf_stream->uncompress, xf_stream->name_on_disk);
		xf_stream->fp = popen(command, "r");
		if (command != command_buf)
			free(command);
	} else {
		/* uncompressed file */
		xf_stream->fp = fopen(xf_stream->name_on_disk, "rb");
	}

	return xf_stream->fp;
}

/*
 * Close a file stream opened by open_file(). Do not free xf_stream.
 */
int
close_stream(struct xfig_stream *restrict xf_stream)
{
	if (xf_stream->fp == NULL)
		return -1;

	if (xf_stream->uncompress[0] == '\0') {
		/* a regular file */
		return fclose(xf_stream->fp);
	} else {
		/* a pipe */
		char	trash[BUFSIZ];
		/* for a pipe, must read everything or
		   we'll get a broken pipe message */
		while (fread(trash, (size_t)1, (size_t)BUFSIZ, xf_stream->fp) ==
				(size_t)BUFSIZ)
			;
		return pclose(xf_stream->fp);
	}
}

FILE *
rewind_stream(struct xfig_stream *restrict xf_stream)
{
	if (xf_stream->fp == NULL)
		return NULL;

	if (xf_stream->uncompress[0] == '\0') {
		/* a regular file */
		rewind(xf_stream->fp);
		return xf_stream->fp;
	} else  {
		/* a pipe */
		(void)close_stream(xf_stream);
		/* if, in the meantime, e.g., the file on disk
		   was uncompressed, change the filetype. */
		return open_stream(xf_stream->name, xf_stream);
	}
}

/*
 * Have xf_stream->content either point to a regular file containing the
 * uncompressed content of xf_stream->name, or to xf_stream->name_on_disk, if
 * name_on_disk is not compressed.
 * Use after a call to open_stream():
 *	struct xfig_stream	xf_stream;
 *	open_stream(name, &xf_stream);
 *	close_stream(&xf_stream);
 *	uncompressed_content(&xf_stream)
 *	do_something(xf_stream->content);
 *	free_stream(&xf_stream);
 */
int
uncompressed_content(struct xfig_stream *restrict xf_stream)
{
	int		ret = -1;
	int		len;
	char		command_buf[256];
	char		*command = command_buf;
	FILE		*f;

	if (*xf_stream->uncompress == '\0') {
		xf_stream->content = xf_stream->name_on_disk;
		return 0;
	}

	/* create a temporary file */
	strcpy(xf_stream->content, "f2dXXXXXX");
	f = xtmpfile(&xf_stream->content, sizeof xf_stream->content_buf);
	if (f)
		fclose(f);
	else
		return ret;

	/* uncompress to a temporary file */
#define	UNCOMPRESS_FMT	"%s '%s' >%s"
	len = snprintf(command, sizeof command_buf, UNCOMPRESS_FMT,
			xf_stream->uncompress, xf_stream->name_on_disk,
			xf_stream->content);
	if (len < 0) {
		err_msg("Unable to write command string, " UNCOMPRESS_FMT,
			    xf_stream->uncompress, xf_stream->name_on_disk,
			    xf_stream->content);
		return ret;
	}
	if (len >= (int)(sizeof command_buf)) {
		if ((command = malloc(len + 1)) == NULL) {
			put_msg(Err_mem);
			return ret;
		}
		sprintf(command, UNCOMPRESS_FMT, xf_stream->uncompress,
				xf_stream->name_on_disk, xf_stream->uncompress);
	}

	if (system(command) == 0)
		ret = 0;
	else
		err_msg("Could not uncompress %s, command, %s",
				xf_stream->name_on_disk, command);

	if (command != command_buf)
		free(command);
	return ret;
}
