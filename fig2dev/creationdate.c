/*
 * Fig2dev: Translate Fig code to various Devices
 * Copyright (c) 1991 by Micah Beck
 * Parts Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2015 by Brian V. Smith
 * Parts Copyright (c) 2015-2023 by Thomas Loimer
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
 * creationdate.c: provide the creation time and enable reproducibe builds
 * Author: Thomas Loimer, 2016-06-28
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "creationdate.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef	HAVE_STRERROR
#include <errno.h>
#endif
#include <time.h>
#include <limits.h>


static struct tm*
parse_time(void)
{
	time_t			now;

#ifdef	HAVE_STRERROR
	char			*source_date_epoch;
	unsigned long long	epoch;
	char			*endptr;

	source_date_epoch = getenv("SOURCE_DATE_EPOCH");
	if (source_date_epoch) {
		errno = 0;
		epoch = strtoull(source_date_epoch, &endptr, 10);
		if ((errno == ERANGE && (epoch == ULLONG_MAX || epoch == 0)) ||
				(errno != 0 && epoch == 0)) {
			fprintf(stderr,"Environment variable SOURCE_DATE_EPOCH:"
				       " strtoull: %s\n", strerror(errno));
		} else if (endptr == source_date_epoch) {
			fprintf(stderr,"Environment variable SOURCE_DATE_EPOCH:"
					" No digits were found: %s\n", endptr);
		} else if (*endptr != '\0') {
			fprintf(stderr,"Environment variable SOURCE_DATE_EPOCH:"
					" Trailing garbage: %s\n", endptr);
		} else if (epoch > ULONG_MAX) {
			fprintf(stderr,"Environment variable SOURCE_DATE_EPOCH:"
					" value must be smaller than or equal "
					"to: %lu but was found to be: %llu \n",
					ULONG_MAX, epoch);
		} else {
			/* no errors, epoch is valid */
			now = epoch;
			return gmtime(&now);
		}
	}
#endif

	/* fall trough on errors or !source_date_epoch */
	time(&now);
	return localtime(&now);
}

static struct tm*
get_time(void)
{
	static struct tm	time;
	static int		initialized = 0;
	if (!initialized) {
		time = *parse_time();
		initialized = 1;
	}
	return &time;
}

int
creation_date(char *restrict buf)
{
	if (strftime(buf, CREATION_TIME_LEN, "%F %H:%M:%S", get_time()))
		return 1;
	else
		return 0;
}

int
creation_date_pdfmark(char *restrict buf)
{
	/*
	 * Pdfmark format should be D:YYYYMMDDHHmmSSOHH’mm’.
	 * Timezone offset (O...) may be omitted
	 */
	if (strftime(buf, CREATION_TIME_LEN, "D:%Y%m%d%H%M%S", get_time()))
		return 1;
	else
		return 0;
}
