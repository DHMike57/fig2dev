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
 * Convert text strings between charsets and / or encodings.
 * Author: Thomas Loimer
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "textconvert.h"

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_ICONV
#include <iconv.h>
#endif
#ifdef HAVE_NL_LANGINFO
#include <langinfo.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include "messages.h"

int	only_ascii = 1;

#ifdef HAVE_ICONV
static iconv_t	cd;
#endif

static int
get_local_charset(char *charset, size_t size)
{
#ifdef HAVE_NL_LANGINFO
	char	*c = nl_langinfo(CODESET);
	size_t	size_c;

	size_c = strlen(c) + 1;
	if (size_c > size) {
		put_msg("Cannot deal with charset names longer than %zd bytes.",
				size);
		return -1;
	}
	memcpy(charset, c, size_c);
	return 0;
#else
	char	*c;
	if ((c = strchr(setlocale(LC_CTYPE, NULL), '.'))) {
		size_t	size_c = strlen(c + 1) + 1;
		if (size_c > size) {
			put_msg("Cannot deal with charset names longer than %zd"
					" bytes.", size_c);
			return -1;
		}
		memcpy(charset, c + 1, size_c);
		return 0;
	}
	return -1;
#endif
}


int
contains_non_ascii(char *str)
{
	const unsigned char	mask = ~0x7f;
	unsigned char		*c;
	int			ret = 0;
	for (c = (unsigned char *)str; *c != '\0'; ++c) {
		if (*c & mask) {
			ret = 1;
			break;
		}
	}
	return ret;
}

/*
 * Check, whether a conversion between in- and output_charset is necessary.
 * Return 0 if not necessary or not possible, 1 otherwise.
 */
int
check_conversion(const char *restrict output_charset,
		const char *restrict input_charset)
{
	char	inbuf[32];
	char	outbuf[32];
	char	*in;
	char	*out;

	if (only_ascii)
		return 0;

	if (!input_charset && !output_charset)
		return 0;

	if ((!input_charset && get_local_charset(inbuf, sizeof inbuf)) ||
			(!output_charset &&
				get_local_charset(outbuf, sizeof outbuf)))
		return 0;

	if (input_charset)
		in = (char *)input_charset;
	else
		in = inbuf;
	if (output_charset)
		out = (char *)output_charset;
	else
		out = outbuf;

	/* This may not work, e.g.,"utf8" versus "UTF-8",
	   or latin1 versus ISO-8859-1 would fail. */
	if (!strcasecmp(in, out))
		return 0;

#ifdef HAVE_ICONV
	if ((cd = iconv_open(out, in)) == (iconv_t)-1) {
		fprintf(stderr, "Unable to convert from %s to %s character "
				"set.\n", in, out);
		return 0;
	}
	return 1;
#else
	fprintf(stderr, "iconv() not available, conversion between character "
			"sets, from %s to %s, is not possible.\n", in, out);
	return 0;
#endif
}

/*
 * Convert the character sequence given in the string in to the newly allocated
 * string *out. Use the conversion specifier previously determined by a call to
 * check_conversion().
 * Return 0 on success.
 * On error, return the number of remaining unconverted input characters.
 * The caller should free *out after use.
 */
int
convert(char **restrict out, char *restrict in, size_t inlen)
{
	int	stat;
	size_t	converted;
	size_t	out_remain;
	size_t	out_size;
	size_t	in_remain;
	char	*inpos;
	char	*outpos;

	if (inlen == 0)
		return 0;

	out_size = inlen > 8 ? 2 * inlen : 16;
	if (!(*out = malloc(out_size))) {
		put_msg(Err_mem);
		exit(EXIT_FAILURE);
	}
	out_remain = out_size;
	/* the final '\0' needs to be part of the conversion */
	in_remain = inlen + 1;
	inpos = in;
	outpos = *out;
	errno = 0;
	while ((converted = iconv(cd, &inpos, &in_remain, &outpos, &out_remain))
			&& in_remain != 0 && errno == E2BIG) {
		/* output size too small */
		size_t		to_outpos = (size_t)(outpos - *out);
		size_t		mult = 2;
		size_t		old_size = out_size;
		/* compute the expansion factor from in to out */
		if (inpos - in > 0) {
			mult = to_outpos / (size_t)(inpos - in) + 1;
			if (mult < 2)
				mult = 2;
		}
		if (!(*out = realloc(*out, out_size *= mult))) {
			put_msg(Err_mem);
			exit(EXIT_FAILURE);
		}
		/* continue the conversion from where it stopped */
		outpos = *out + to_outpos;
		out_remain += out_size - old_size;
		errno = 0;
	}

	if (converted == (size_t)-1) {
		if (errno == EINVAL)
			put_msg("The input string %s prematurely terminated.",
					in);
		else
			err_msg("Error converting string %s", in);
	}

	/*
	 * man iconv(3) notes, the last call to iconv() should be with inbuf set
	 * to NULL, in order to flush out any partially converted input.
	 * The following would also append a shift sequence to return to the
	 * initial conversion state.
	 */
	stat = in_remain;
	while (iconv(cd, NULL, &in_remain, &outpos, &out_remain) == (size_t)-1
			&& errno == E2BIG) {
		const size_t	enlarge = 8;
		size_t		to_outpos = (size_t)(outpos - *out);
		if (!(*out = realloc(*out, out_size += enlarge))) {
			put_msg(Err_mem);
			exit(EXIT_FAILURE);
		}
		outpos = *out + to_outpos;
		out_remain += enlarge;
	}
	/* one could realloc() *out to the minimum necessary; However, currently
	 * the caller immediately free's *out anyhow. */
	return stat;
}

/*
 * Convert utf8 to latin1. Code points beyond latin 1 are silently omitted.
 * Return -1 if characters beyond latin1 are found or errors are encountered,
 * 0 if all went smoothly.
 */
int
convertutf8tolatin1(char *restrict str)
{
	unsigned char	*c;
	unsigned char	*d;
	int		stat = 0;

	d = (unsigned char *)str;
	for (c = (unsigned char *)str; *c; ++c) {
		if (!(*c & 0x80/* 1000 0000 */)) {	/* ascii */
			if (d == c)
				++d;
			else	
				*d++ = *c;
		} else {				/* above ascii */
			if ((*c & 0xd0/* 1110 0000 */) == 0xc0/* 1100 0000*/ &&
					(*d = *c & 0x3f/* 0011 1111 */) < 4 &&
					*++c) {
				*d = *d << 6 | (*c & 0x3f);
				++d;
			} else {
				stat = -1;
				/* skip any sequence representing larger
				   codepoints */
				while (*c && *c & 0x80)
					++c;
			}
		}
	}
	*d = '\0';
	return stat;
}
