/*
  Copyright (C) 2011 Joseph A. Adams (joeyadams3.14159@gmail.com)
  All rights reserved.

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

/*
 * NOTE: This file has been modified from its original form to remove
 * functions that we (AppNexus) do not use.
 */

#include <assert.h>

#include "an_charset.h"

ACF_EXPORT struct an_utf8_stats
an_utf8_stats_get(const char *str, size_t byte_length)
{
	struct an_utf8_stats rc = { .is_valid = false };

	const char *s = str;
	const char *e = str + byte_length;
	int len;

	for (; s < e; s += len) {
		len = an_is_utf8(s, e);
		if (len == 0) {
			return rc;
		}

		if (len > 1) {
			rc.wide_code_point_count++;
		}
		rc.parsed_length += len;
		rc.total_code_point_count++;
	}
	assert(s == e);

	rc.is_valid = true;
	return rc;
}

ACF_EXPORT bool
an_utf8_validate(const char *str, size_t byte_length)
{
	const struct an_utf8_stats s = an_utf8_stats_get(str, byte_length);

	return s.is_valid;
}

/*
 * This function implements the syntax given in RFC3629, which is
 * the same as that given in The Unicode Standard, Version 6.0.
 *
 * It has the following properties:
 *
 *  * All codepoints U+0000..U+10FFFF may be encoded,
 *    except for U+D800..U+DFFF, which are reserved
 *    for UTF-16 surrogate pair encoding.
 *  * UTF-8 byte sequences longer than 4 bytes are not permitted,
 *    as they exceed the range of Unicode.
 *  * The sixty-six Unicode "non-characters" are permitted
 *    (namely, U+FDD0..U+FDEF, U+xxFFFE, and U+xxFFFF).
 */
ACF_EXPORT int
an_is_utf8(const char *s, const char *e)
{
	unsigned char c = *s++;

	if (c <= 0x7F) {        /* 00..7F */
		return 1;
	} else if (c <= 0xC1) { /* 80..C1 */
		/* Disallow overlong 2-byte sequence. */
		return 0;
	} else if (c <= 0xDF) { /* C2..DF */
		/* Make sure the character isn't clipped. */
		if (e - s < 1) {
			return 0;
		}

		/* Make sure subsequent byte is in the range 0x80..0xBF. */
		if (((unsigned char)*s++ & 0xC0) != 0x80) {
			return 0;
		}

		return 2;
	} else if (c <= 0xEF) { /* E0..EF */
		/* Make sure the character isn't clipped. */
		if (e - s < 2) {
			return 0;
		}

		/* Disallow overlong 3-byte sequence. */
		if (c == 0xE0 && (unsigned char)*s < 0xA0) {
			return 0;
		}

		/* Disallow U+D800..U+DFFF. */
		if (c == 0xED && (unsigned char)*s > 0x9F) {
			return 0;
		}

		/* Make sure subsequent bytes are in the range 0x80..0xBF. */
		if (((unsigned char)*s++ & 0xC0) != 0x80) {
			return 0;
		}

		if (((unsigned char)*s++ & 0xC0) != 0x80) {
			return 0;
		}

		return 3;
	} else if (c <= 0xF4) { /* F0..F4 */
		/* Make sure the character isn't clipped. */
		if (e - s < 3) {
			return 0;
		}

		/* Disallow overlong 4-byte sequence. */
		if (c == 0xF0 && (unsigned char)*s < 0x90) {
			return 0;
		}

		/* Disallow codepoints beyond U+10FFFF. */
		if (c == 0xF4 && (unsigned char)*s > 0x8F) {
			return 0;
		}

		/* Make sure subsequent bytes are in the range 0x80..0xBF. */
		if (((unsigned char)*s++ & 0xC0) != 0x80) {
			return 0;
		}

		if (((unsigned char)*s++ & 0xC0) != 0x80) {
			return 0;
		}

		if (((unsigned char)*s++ & 0xC0) != 0x80) {
			return 0;
		}

		return 4;
	} else {                /* F5..FF */
		return 0;
	}
}
