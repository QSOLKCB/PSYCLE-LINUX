/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
**
** Original code by Daniel.Veillard@w3.org
**
** part of encoding.c libxml2
**
** psycle changes: modified int to intptr_t
**
** Original code for IsoLatin1and UTF - 16 by "Martin J. Duerst" < duerst@w3.org >
**
** See Copyright for the status of this software.
**
** Daniel.Veillard@w3.org
*/

#include "../../detail/prefix.h"


#include "encoding.h"

/* std */
#include <assert.h>
#include <stdio.h>
/* platform */
#include "../../detail/portable.h"


/**
 * isolat1ToUTF8:
 * @out:  a pointer to an array of bytes to store the result
 * @outlen:  the length of @out
 * @in:  a pointer to an array of ISO Latin 1 chars
 * @inlen:  the length of @in
 *
 * Take a block of ISO Latin 1 chars in and try to convert it to an UTF-8
 * block of chars out.
 * Returns 0 if success, or -1 otherwise
 * The value of @inlen after return is the number of octets consumed
 *     as the return value is positive, else unpredictiable.
 * The value of @outlen after return is the number of ocetes consumed.
 */
intptr_t
isolat1ToUTF8(unsigned char* out, intptr_t* outlen,
	const unsigned char* in, intptr_t* inlen) {
	unsigned char* outstart = out;
	const unsigned char* base = in;
	const unsigned char* processed = in;
	unsigned char* outend = out + *outlen;
	const unsigned char* inend;
	unsigned int c;
	intptr_t bits;

	inend = in + (*inlen);
	while ((in < inend) && (out - outstart + 5 < *outlen)) {
		c = *in++;

		/* assertion: c is a single UTF-4 value */
		if (out >= outend)
			break;
		if (c < 0x80) { *out++ = c;                bits = -6; }
		else { *out++ = ((c >> 6) & 0x1F) | 0xC0;  bits = 0; }

		for (; bits >= 0; bits -= 6) {
			if (out >= outend)
				break;
			*out++ = ((c >> bits) & 0x3F) | 0x80;
		}
		processed = (const unsigned char*)in;
	}
	*outlen = out - outstart;
	*inlen = processed - base;
	return(0);
}


/**
 * UTF8Toisolat1:
 * @out:  a pointer to an array of bytes to store the result
 * @outlen:  the length of @out
 * @in:  a pointer to an array of UTF-8 chars
 * @inlen:  the length of @in
 *
 * Take a block of UTF-8 chars in and try to convert it to an ISO Latin 1
 * block of chars out.
 *
 * Returns 0 if success, -2 if the transcoding fails, or -1 otherwise
 * The value of @inlen after return is the number of octets consumed
 *     as the return value is positive, else unpredictiable.
 * The value of @outlen after return is the number of ocetes consumed.
 */
intptr_t
UTF8Toisolat1(unsigned char* out, intptr_t* outlen,
	const unsigned char* in, intptr_t* inlen) {
	const unsigned char* processed = in;
	const unsigned char* outend;
	const unsigned char* outstart = out;
	const unsigned char* instart = in;
	const unsigned char* inend;
	unsigned int c, d;
	intptr_t trailing;

	if (in == NULL) {
		/*
	 * initialization nothing to do
	 */
		*outlen = 0;
		*inlen = 0;
		return(0);
	}
	inend = in + (*inlen);
	outend = out + (*outlen);
	while (in < inend) {
		d = *in++;
		if (d < 0x80) { c = d; trailing = 0; }
		else if (d < 0xC0) {
			/* trailing byte in leading position */
			*outlen = out - outstart;
			*inlen = processed - instart;
			return(-2);
		}
		else if (d < 0xE0) { c = d & 0x1F; trailing = 1; }
		else if (d < 0xF0) { c = d & 0x0F; trailing = 2; }
		else if (d < 0xF8) { c = d & 0x07; trailing = 3; }
		else {
			/* no chance for this in IsoLat1 */
			*outlen = out - outstart;
			*inlen = processed - instart;
			return(-2);
		}

		if (inend - in < trailing) {
			break;
		}

		for (; trailing; trailing--) {
			if (in >= inend)
				break;
			if (((d = *in++) & 0xC0) != 0x80) {
				*outlen = out - outstart;
				*inlen = processed - instart;
				return(-2);
			}
			c <<= 6;
			c |= d & 0x3F;
		}

		/* assertion: c is a single UTF-4 value */
		if (c <= 0xFF) {
			if (out >= outend)
				break;
			*out++ = c;
		}
		else {
			/* no chance for this in IsoLat1 */
			*outlen = out - outstart;
			*inlen = processed - instart;
			return(-2);
		}
		processed = in;
	}
	*outlen = out - outstart;
	*inlen = processed - instart;
	return(0);
}

/**
 * xmlCheckUTF8: Check utf-8 string for legality.
 * @utf: Pointer to putative utf-8 encoded string.
 *
 * Checks @utf for being valid utf-8. @utf is assumed to be
 * null-terminated. This function is not super-strict, as it will
 * allow longer utf-8 sequences than necessary. Note that Java is
 * capable of producing these sequences if provoked. Also note, this
 * routine checks for the 4-byte maxiumum size, but does not check for
 * 0x10ffff maximum value.
 *
 * Return value: true if @utf is valid.
 **/
int
xmlCheckUTF8(const unsigned char* utf)
{
	intptr_t ix;
	unsigned char c;

	for (ix = 0; (c = utf[ix]);) {
		if (c & 0x80) {
			if ((utf[ix + 1] & 0xc0) != 0x80)
				return(0);
			if ((c & 0xe0) == 0xe0) {
				if ((utf[ix + 2] & 0xc0) != 0x80)
					return(0);
				if ((c & 0xf0) == 0xf0) {
					if ((c & 0xf8) != 0xf0 || (utf[ix + 3] & 0xc0) != 0x80)
						return(0);
					ix += 4;
					/* 4-byte code */
				}
				else
					/* 3-byte code */
					ix += 3;
			}
			else
				/* 2-byte code */
				ix += 2;
		}
		else
			/* 1-byte code */
			ix++;
	}
	return(1);
}

/**
 * xmlGetUTF8Char:
 * @utf:  a sequence of UTF-8 encoded bytes
 * @len:  a pointer to @bytes len
 *
 * Read one UTF8 Char from @utf
 *
 * Returns the char value or -1 in case of error and update @len with the
 *        number of bytes used
 */
int
xmlGetUTF8Char(const unsigned char *utf, int *len) {
    unsigned int c;

    if (utf == NULL)
	goto error;
    if (len == NULL)
	goto error;
    if (*len < 1)
	goto error;

    c = utf[0];
    if (c & 0x80) {
	if (*len < 2)
	    goto error;
	if ((utf[1] & 0xc0) != 0x80)
	    goto error;
	if ((c & 0xe0) == 0xe0) {
	    if (*len < 3)
		goto error;
	    if ((utf[2] & 0xc0) != 0x80)
		goto error;
	    if ((c & 0xf0) == 0xf0) {
		if (*len < 4)
		    goto error;
		if ((c & 0xf8) != 0xf0 || (utf[3] & 0xc0) != 0x80)
		    goto error;
		*len = 4;
		/* 4-byte code */
		c = (utf[0] & 0x7) << 18;
		c |= (utf[1] & 0x3f) << 12;
		c |= (utf[2] & 0x3f) << 6;
		c |= utf[3] & 0x3f;
	    } else {
	      /* 3-byte code */
		*len = 3;
		c = (utf[0] & 0xf) << 12;
		c |= (utf[1] & 0x3f) << 6;
		c |= utf[2] & 0x3f;
	    }
	} else {
	  /* 2-byte code */
	    *len = 2;
	    c = (utf[0] & 0x1f) << 6;
	    c |= utf[1] & 0x3f;
	}
    } else {
	/* 1-byte code */
	*len = 1;
    }
    return(c);

error:
    *len = 0;
    return(-1);
}
