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
 *	creationdate.c - created 2016-06-28 by Thomas Loimer
 *
 *	* 2017-07-22	remove HAVE_LIMITS_H
 */

#ifdef	HAVE_CONFIG_H
#include "config.h"
#endif
#include "creationdate.h"

#include "fig2dev.h"
/* fig2dev.h includes stdbool-gymnastics;
 * TODO: only include stdbool.h (etc) and the headers below.
 */
/*
#include <stdio.h>
#include <stdlib.h>	strtoull, getenv
#include <time.h>
#ifdef	HAVE_STRERROR
#include <errno.h>
#endif
#ifdef	HAVE_LIMITS_H
#include <limits.h>
#endif
*/

int
creation_date(char *buf)
{
    time_t now;

#ifdef	HAVE_STRERROR
    char *source_date_epoch;
    unsigned long long epoch;
    char *endptr;

    source_date_epoch = getenv("SOURCE_DATE_EPOCH");
    if (source_date_epoch) {
	errno = 0;
	epoch = strtoull(source_date_epoch, &endptr, 10);
	if ((errno == ERANGE && (epoch == ULLONG_MAX || epoch == 0))
		|| (errno != 0 && epoch == 0)) {
	    fprintf(stderr,
		"Environment variable $SOURCE_DATE_EPOCH: strtoull: %s\n",
		strerror(errno));
	} else if (endptr == source_date_epoch) {
	    fprintf(stderr,
		"Environment variable $SOURCE_DATE_EPOCH: No digits were found: %s\n",
		endptr);
	} else if (*endptr != '\0') {
	    fprintf(stderr,
		"Environment variable $SOURCE_DATE_EPOCH: Trailing garbage: %s\n",
		endptr);
	} else if (epoch > ULONG_MAX) {
	    fprintf(stderr,
		"Environment variable $SOURCE_DATE_EPOCH: value must be smaller than or equal to: %lu but was found to be: %llu \n",
		ULONG_MAX, epoch);
	} else {
	    /* no errors, epoch is valid */
	    strftime(buf, CREATION_TIME_LEN, "%c", gmtime((time_t *)&epoch));
	    return true;
	}
    }
#endif

    /* fall trough on errors or !source_date_epoch */
    time(&now);
    if (strftime(buf, CREATION_TIME_LEN, "%c", localtime(&now)))
	return true;
    else
	return false;
}
