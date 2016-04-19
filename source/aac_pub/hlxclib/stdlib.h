/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: stdlib.h,v 1.10 2004/07/09 18:21:09 hubbe Exp $
 * 
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
 * 
 * The contents of this file, and the files included with this file,
 * are subject to the current version of the RealNetworks Public
 * Source License (the "RPSL") available at
 * http://www.helixcommunity.org/content/rpsl unless you have licensed
 * the file under the current version of the RealNetworks Community
 * Source License (the "RCSL") available at
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL
 * will apply. You may also obtain the license terms directly from
 * RealNetworks.  You may not use this file except in compliance with
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for
 * the rights, obligations and limitations governing use of the
 * contents of the file.
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above. If you wish to allow use of your version of
 * this file only under the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete the provisions above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
 * 
 * This file is part of the Helix DNA Technology. RealNetworks is the
 * developer of the Original Code and owns the copyrights in the
 * portions it created.
 * 
 * This file, and the files included with this file, is distributed
 * and made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS
 * ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET
 * ENJOYMENT OR NON-INFRINGEMENT.
 * 
 * Technology Compatibility Kit Test Suite(s) Location:
 *    http://www.helixcommunity.org/content/tck
 * 
 * Contributor(s):
 * 
 * ***** END LICENSE BLOCK ***** */

#ifndef HLXSYS_STDLIB_H
#define HLXSYS_STDLIB_H

#include "hxtypes.h"

#if defined(_OPENWAVE)
// XXXSAB Include compiler <stdlib.h> so we can modify it???
#ifdef _OPENWAVE_SIMULATOR
#ifndef _WIN32
#define STDLIB_UNDEF_WIN32
#define _WIN32
#endif /* _WIN32 */
#endif /* _OPENWAVE_SIMULATOR */

#include <stdlib.h>
#undef itoa                     // just in case

#ifdef STDLIB_UNDEF_WIN32
#undef _WIN32
#undef STDLIB_UNDEF_WIN32
#endif /* STDLIB_UNDEF_WIN32 */

// XXXSAB Define malloc()/free() wrappers for Openwave in here???
#else
#include <stdlib.h>
#endif

char* __helix_itoa(int val, char *str, int radix);
char* __helix_i64toa(INT64 val, char *str, int radix);
INT64 __helix_atoi64(char* str);
void* __helix_bsearch( const void *key, const void *base, size_t num, 
		       size_t width, 
		       int (  *compare ) ( const void *elem1, 
					   const void *elem2 ) );
int __helix_remove(const char* pPath);
int __helix_putenv(const char* pStr);
char* __helix_getenv(const char* pName);

#if defined(_WINDOWS) && !defined(_OPENWAVE)

#if !defined(WIN32_PLATFORM_PSPC)
_inline char*
i64toa(INT64 val, char* str, int radix)
{
    return _i64toa(val, str, radix);
}

#else /* !defined(WIN32_PLATFORM_PSPC) */

_inline
int remove(const char* pPath)
{
    return __helix_remove(pPath);
}

_inline
char* getenv(const char* pName)
{
    return __helix_getenv(pName);
}

#define i64toa __helix_i64toa
#define itoa __helix_itoa

_inline
void* bsearch( const void *key, const void *base, size_t num, 
	       size_t width, 
	       int ( *compare ) ( const void *elem1, 
				  const void *elem2 ) )
{
    return __helix_bsearch(key, base, num, width, compare);
}
#endif /* !defined(WIN32_PLATFORM_PSPC) */

_inline INT64
atoi64(const char* str)
{
    return _atoi64(str);
}
#endif /* _WINDOWS */

#if defined (_MACINTOSH) 

#define itoa __helix_itoa
#define i64toa __helix_i64toa
#define atoi64 __helix_atoi64

#endif /* _MACINTOSH */

#if defined (_UNIX) && !defined (__QNXNTO__)

// Convert integer to string

#define itoa __helix_itoa
#define i64toa __helix_i64toa
#define atoi64 __helix_atoi64

#endif /* _UNIX */

#if defined(_SYMBIAN)

#define itoa __helix_itoa
#define i64toa __helix_i64toa
#define atoi64 __helix_atoi64
#define putenv __helix_putenv

#endif

#if defined(_OPENWAVE)

#define itoa(v,s,r) __helix_itoa((v),(s),(r))
#define i64toa(v,s,r) __helix_i64toa((v),(s),(r))
#define atoi64(s) __helix_atoi64((s))
#define putenv __helix_putenv

__inline int remove(const char* pPath)
{
    return __helix_remove(pPath);
}

#endif // _OPENWAVE

#endif /* HLXSYS_STDLIB_H */
