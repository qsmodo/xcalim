/*
 *	This file is extracted from the BSD/386 sources.
 *	It is distributable subject to the Copyright notice below
 *	The code has been change for xcalim.c purposes
 *	The original code is also distributed with xcalim in case your
 *	system does not have the strftime() function
 *	strfdate.c	1.3	11/4/93
 */


/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <sys/time.h>
#include <string.h>
#include <X11/Intrinsic.h>
#include <X11/Xos.h>
#include <X11/StringDefs.h>
#include "xcalim.h"

static char **afmt = appResources.sday;

static char **Afmt = appResources.day;

static char **bfmt = appResources.smon;
static char **Bfmt = appResources.mon;

static size_t gsize;
static char *pt;
static int _add(), _conv();
static size_t _fmt();



size_t
strfdate(s, maxsize, format, d)
	char *s;
	size_t maxsize;
	char *format;
	Date *d;
{
	pt = s;
	if ((gsize = maxsize) < 1)
		return(0);
	if (_fmt(format, d)) {
		*pt = '\0';
		return(maxsize - gsize);
	}
	return(0);
}

static size_t
_fmt(format, d)
	register char *format;
	Date *d;
{
	for (; *format; ++format) {
		if (*format == '%')
			switch(*++format) {
			case '\0':
				--format;
				break;
			case 'A':
				if (d->wday < 0 || d->wday > 6)
					return(0);
				if (!_add(Afmt[d->wday]))
					return(0);
				continue;
			case 'a':
				if (d->wday < 0 || d->wday > 6)
					return(0);
				if (!_add(afmt[d->wday]))
					return(0);
				continue;
			case 'B':
				if (d->month < 0 || d->month > 11)
					return(0);
				if (!_add(Bfmt[d->month]))
					return(0);
				continue;
			case 'b':
			case 'h':
				if (d->month < 0 || d->month > 11)
					return(0);
				if (!_add(bfmt[d->month]))
					return(0);
				continue;
			case 'D':
				if (!_fmt("%m/%d/%y", d))
					return(0);
				continue;
			case 'd':
				if (!_conv(d->day, 2, '0'))
					return(0);
				continue;
			case 'e':
				if (!_conv(d->day, 2, ' '))
					return(0);
				continue;
			case 'm':
				if (!_conv(d->month + 1, 2, '0'))
					return(0);
				continue;
			case 'n':
				if (!_add("\n"))
					return(0);
				continue;
			case 't':
				if (!_add("\t"))
					return(0);
				continue;
			case 'x':
				if (!_fmt("%m/%d/%y", d))
					return(0);
				continue;
			case 'y':
				if (!_conv((d->year)
				    % 100, 2, '0'))
					return(0);
				continue;
			case 'Y':
				if (!_conv(d->year, 4, '0'))
					return(0);
				continue;
			case '%':
			/*
			 * X311J/88-090 (4.12.3.5): if conversion char is
			 * undefined, behavior is undefined.  Print out the
			 * character itself as printf(3) does.
			 */
			default:
				break;
		}
		if (!gsize--)
			return(0);
		*pt++ = *format;
	}
	return(gsize);
}

static
_conv(n, digits, pad)
	int n, digits;
	char pad;
{
	static char buf[10];
	register char *p;

	for (p = buf + sizeof(buf) - 2; n > 0 && p > buf; n /= 10, --digits)
		*p-- = n % 10 + '0';
	while (p > buf && digits-- > 0)
		*p-- = pad;
	return(_add(++p));
}

static
_add(str)
	register char *str;
{
	for (;; ++pt, --gsize) {
		if (!gsize)
			return(0);
		if (!(*pt = *str++))
			return(1);
	}
}
