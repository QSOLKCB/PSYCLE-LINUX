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

#if !defined(ENCODING_H)
#define ENCODING_H

#include "../../detail/psydef.h"

#ifdef __cplusplus
extern "C" {
#endif

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
		const unsigned char* in, intptr_t* inlen);

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
		const unsigned char* in, intptr_t* inlen);

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
xmlCheckUTF8(const unsigned char* utf);

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
xmlGetUTF8Char(const unsigned char *utf, int *len);

#ifdef __cplusplus
}
#endif

#endif /* ENCODING_H */
