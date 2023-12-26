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


/*
 * Offer two conversion functions: One for the conversion of text strings from
 * the encoding of a lecacy .fig file to utf8, another one for the conversion of
 * file names referred to in utf8-encoded fig files to the local encoding.
 * The former conversion is necessary for .fig files that do not contain the
 * "#encoding: UTF-8" line and which were written in a non-utf8 locale
 * environment. The latter conversion is necessary if the locale environment is
 * not utf8, but a modern utf8-encoded fig file has embedded images and refers
 * to the file names of these embedded images.
 *
 * The two conversions are named text-conversion and file-conversion.
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
#else
#include <locale.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include "fig2dev.h"		/* input_encoding */
#include "messages.h"


#ifdef HAVE_ICONV
typedef iconv_t
#else
typedef int
#endif
		xf_conv;

/*** specification of the two different conversions ***/

/* refer to the two different types of conversion */
enum conv_type {
	conv_file,
	conv_text,
	num_types
};

/* store the state of the conversion types */
static struct {
	int	needed;
	xf_conv	cd;
} conv_spec[num_types] = {
	{-1,	(xf_conv)-1},
	{-1,	(xf_conv)-1}
};

/* the desired output encodings for the two conversions;
   input is always input_encoding */
static const char	*const out_encoding[num_types] = {
	NULL,
	"UTF-8"
};

/*** end specification of conversions ***/


#define CONV_UNINITIALIZED	(-1)
#define CONV_NOTNEEDED		0
#define CONV_NEEDED		1


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


char *
conv_non_ascii(const char *restrict str)
{
	const unsigned char	mask = ~0x7f;
	unsigned char		*c;

	for (c = (unsigned char *)str; *c != '\0'; ++c)
		if (*c & mask)
			return  (char *)c;
	return NULL;
}

/*
 * Check, whether a conversion between in- and output_charset is necessary.
 * Retrieve the local charset if input_charset or output_charset is NULL.
 * The strings naming input_charset and output_charset must fit in 31 chars.
 * Return 0 if not necessary or not possible, 1 otherwise.
 */
static int
check_conversion(const char *restrict output_charset,
		const char *restrict input_charset, xf_conv *cd)
{
#ifndef HAVE_ICONV
	(void)cd;
#endif
	char	inbuf[32];
	char	outbuf[32];
	char	*in;
	char	*out;

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
	if ((*cd = iconv_open(out, in)) == (iconv_t)-1) {
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
 * string *out, inlen = strlen(in). Use the conversion specifier previously
 * determined by a call to check_conversion().
 * Return 0 on success.
 * On error, return the number of remaining unconverted input characters.
 * The caller should free *out after use.
 */
static int
convert(char **restrict out, char *restrict in, size_t inlen, xf_conv cd)
{
	int	stat = 0;
#ifdef HAVE_ICONV
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
#else
	(void)inlen;
	(void)cd;
	*out = in;
#endif /* HAVE_ICONV */
	return stat;
}

static int
conversion(char **restrict out, char *restrict in, size_t inlen,
		enum conv_type type)
{
	char	*c;

	if (!(c = conv_non_ascii(in)))
		goto no_conversion;

	/* Lazily only initialize conversion if a conversion character is found.
	 */
	if (conv_spec[type].needed == CONV_UNINITIALIZED)
		conv_spec[type].needed = check_conversion(out_encoding[type],
				input_encoding, &conv_spec[type].cd);

	if (conv_spec[type].needed == CONV_NOTNEEDED)
		goto no_conversion;

	return convert(out, in, inlen, conv_spec[type].cd);

no_conversion:
	*out = in;
	return 0;
}

/*
 * Convert the string in to out, inlen = strlen(in). The encoding for out is
 * UTF-8 for conv_textstring() and NULL (= locale encoding) for conv_filename().
 * Return 0 on success, the number of unconverted bytes on error.
 * If conversion is not necessary, *out == in and 0 is returned.
 * Free out after use.
 */
int
conv_textstring(char **restrict out, char *restrict in, size_t inlen)
{
	return conversion(out, in, inlen, conv_text);
}

int
conv_filename(char **restrict out, char *restrict in, size_t inlen)
{
	return conversion(out, in, inlen, conv_file);
}

/*
 * Return the first character in a text string that is utf-8 encoded.
 * The text string in the fig file is only in utf-8 if no text conversion is
 * needed, otherwise it is probably 8-bit encoded.
 * Return NULL if none is found, or the string is not utf8-encoded.
 */
char *
conv_textisutf8(const char *restrict str)
{
	char	*c;
	if (!(c = conv_non_ascii(str)))
		return c;

	if (conv_spec[conv_text].needed == CONV_UNINITIALIZED)
		conv_spec[conv_text].needed =
			check_conversion(out_encoding[conv_text],
					input_encoding,
					&conv_spec[conv_text].cd);

	if (conv_spec[conv_text].needed == CONV_NOTNEEDED)
		return NULL;
	else
		return c;
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
