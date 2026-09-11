#if !defined(STRCASESTR_H)
#define STRCASESTR_H

#include "psydef.h"
#include "os.h"

#ifdef DIVERSALIS__OS__MICROSOFT

/*
** musl as a whole is licensed under the following standard MIT license:
** Copyright © 2005-2012 Rich Felker
** https://github.com/BlankOn/musl
**
** Permission is hereby granted, free of charge, to any person obtaining
** a copy of this software and associated documentation files (the
** "Software"), to deal in the Software without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Software, and to
** permit persons to whom the Software is furnished to do so, subject to
** the following conditions:
**
** The above copyright notice and this permission notice shall be
** included in all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <string.h>
#include <ctype.h>

#ifdef __cplusplus
extern "C" {
#endif

INLINE int strncasecmp(const char* _l, const char* _r, size_t n)
{
	const unsigned char* l = (const unsigned char*)_l, * r = (const unsigned char*)_r;
	if (!n--) return 0;
	for (; *l && *r && n && (*l == *r || tolower(*l) == tolower(*r)); l++, r++, n--);
	return tolower(*l) - tolower(*r);
}

INLINE char* strcasestr(const char* h, const char* n)
{
	size_t l = strlen(n);
	for (; *h; h++) if (!strncasecmp(h, n, l)) return (char*)h;
	return 0;
}

#ifdef __cplusplus
}
#endif

#else
#include <string.h>
#endif

#endif /* STRCASESTR_H */
